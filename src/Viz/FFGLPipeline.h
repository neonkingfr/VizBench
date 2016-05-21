#ifndef _FFGLPipeline_H
#define _FFGLPipeline_H

#include <sys/stat.h>

class FFGLPipeline {
public:
	FFGLPipeline() {
		m_pipeline_enabled = false;
		m_sidmin = 0;
		m_sidmax = MAX_SESSIONID;
		m_vizserver = VizServer::GetServer();

	};

	bool do_ffgl_plugin(FFGLPluginInstance* plugin, int which); // which: 0 = first one, 1 = middle, 2 = last, 3 = one and only one, 4 none
	void doPipeline(int window_width, int window_height);
	FFGLPluginInstance* FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string viztag);
	void paintTexture();
	void setSidrange(int sidmin, int sidmax);
	void applyAllPlugins(std::string meth, cJSON* params);
	void setEnableInput(bool onoff);

	void clearPluginlist() {
		DEBUGPRINT1(("FFGLPipeline.clear is called"));
		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
			DEBUGPRINT1(("--- deleteing FFGLPluginInstance *it=%ld", (long)(*it)));
			delete *it;
		}
		m_pluginlist.clear();
	}
	FFGLPluginInstance* find_plugin(std::string viztag, bool required = false) {
		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
			FFGLPluginInstance* p = *it;
			if (viztag == p->viztag()) {
				return *it;
			}
		}
		if (required) {
			throw NosuchException("There is no viztag '%s' in pipeline %s",viztag.c_str(),m_piname.c_str());
		}
		return NULL;
	}
	void delete_plugin(std::string viztag) {
		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end();) {
			if (viztag == (*it)->viztag()) {
				// DEBUGPRINT1(("--- deleteing BB FFGLPluginInstance *it=%ld", (long)(*it)));
				delete *it;
				it = m_pluginlist.erase(it);
			}
			else {
				it++;
			}
		}
	}
	void append_plugin(FFGLPluginInstance* p) {
		m_pluginlist.insert(m_pluginlist.end(), p);
	}
	void shuffle() {
		size_t sz = m_pluginlist.size();
		// Just swap things randomly
		for (size_t n = 1; n < sz; n++) {
			if ((rand() % 2) == 0) {
				if (m_pluginlist[n - 1]->isMoveable() && m_pluginlist[n]->isMoveable()) {
					FFGLPluginInstance* t = m_pluginlist[n - 1];
					m_pluginlist[n - 1] = m_pluginlist[n];
					m_pluginlist[n] = t;
				}
			}
		}
	}
	void randomize() {
		size_t sz = m_pluginlist.size();
		// Just swap things randomly
		for (size_t n = 1; n < sz; n++) {
			int n1 = rand() % sz;
			int n2 = rand() % sz;
			if (n1 != n2) {
				if (m_pluginlist[n1]->isMoveable() && m_pluginlist[n2]->isMoveable()) {
					FFGLPluginInstance* t = m_pluginlist[n1];
					m_pluginlist[n1] = m_pluginlist[n2];
					m_pluginlist[n2] = t;
				}
			}
		}
	}
	void swap(int n1, int n2) {
		FFGLPluginInstance* t = m_pluginlist[n1];
		m_pluginlist[n1] = m_pluginlist[n2];
		m_pluginlist[n2] = t;
	}
	void moveplugin(std::string viztag, int places) {
		size_t sz = m_pluginlist.size();
		bool up = places < 0;
		if (up) {
			places = -places;
		}
		for (int t = 0; t < places; t++) {
			for (size_t n = 0; n < sz; n++) {
				if (viztag == m_pluginlist[n]->viztag()) {
					if (up) {
						if (n > 0) {
							swap(n - 1, n);
						}
					}
					else {
						if (n < (sz - 1)) {
							swap(n + 1, n);
						}
					}
					break;
				}
			}
		}
	}

	FFGLPluginInstance* newPluginInstance(FFGLPluginDef* plugin, std::string viztag) {
		FFGLPluginInstance* np = new FFGLPluginInstance(plugin, viztag);
		// DEBUGPRINT(("----- MALLOC new FFGLPluginInstance"));
		if (np->InstantiateGL(&fboViewport) != FF_SUCCESS) {
			delete np;
			throw NosuchException("Unable to InstantiateGL !?");
		}
		return np;
	}

	FFGLPluginInstance* addToPipeline(std::string pluginName, std::string viztag, bool enable, cJSON* params) {

		// See if this viztag is already used in the pipeline
		if (find_plugin(viztag) != NULL) {
			DEBUGPRINT(("Plugin with viztag '%s' already exists", viztag.c_str()));
			return NULL;
		}

		FFGLPluginDef* plugindef = findffglplugindef(pluginName);
		if (plugindef == NULL) {
			throw NosuchException("There is no plugin named '%s'", pluginName.c_str());
			// DEBUGPRINT(("There is no plugin named '%s'",pluginName.c_str()));
			// return NULL;
		}

		FFGLPluginInstance* np = newPluginInstance(plugindef, viztag);

		// If the plugin's first parameter is "viztag", set it
		int pnum = np->plugindef()->getParamNum("viztag");
		if (pnum >= 0) {
			np->setparam("viztag", viztag);
		}

		// Add it to the end of the pipeline
		append_plugin(np);

		if (params) {
			for (cJSON* pn = params->child; pn != NULL; pn = pn->next) {
				NosuchAssert(pn->type == cJSON_Object);
				std::string nm = jsonNeedString(pn, "name", "");
				if (nm == "") {
					throw NosuchException("Missing parameter name");
				}
				cJSON *t = cJSON_GetObjectItem(pn, "value");
				if (t == NULL) {
					throw NosuchException("Missing parameter value");
				}
				if (t->type == cJSON_String) {
					// In the saved pipeline json, it saves a "vtag" value which
					// is the viztag without the pipeline number prefix.
					if (nm == "vtag" || nm == "viztag") {
						// np->setparam("viztag", viztag);
					}
					else {
						np->setparam(nm, t->valuestring);
					}
				}
				else if (t->type == cJSON_Number) {
					np->setparam(nm, (float)(t->valuedouble));
				}
				else {
					throw NosuchException("addToPipeline unable to handle type=%d", t->type);
				}
			}
		}
		np->setEnable(enable);
		DEBUGPRINT1(("setEnable plugin=%s enable=%d", viztag.c_str(), enable));
		return np;
	}

	bool isEnabled() {
		return m_pipeline_enabled;
	}

	std::string pipelineList(bool only_enabled) {
		std::string r = "[";
		std::string sep = "";

		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
			FFGLPluginInstance* p = *it;

			bool isvizlet = m_vizserver->IsVizlet(p->viztag().c_str());

			std::string isviz;
			if (isvizlet) {
				isviz = std::string(" \"vizlet\": ") + (isvizlet ? "1" : "0") + ", ";
			}
			bool enabled = p->isEnabled();
			if (only_enabled == false || enabled == true) {
				r += (sep + "{ \"plugin\":\"" + p->plugindef()->GetPluginName() + "\","
					+ " \"viztag\":\"" + p->viztag() + "\", "
					+ isviz
					+ " \"moveable\": " + (p->isMoveable() ? "1" : "0") + ","
					+ " \"enabled\": " + (p->isEnabled() ? "1" : "0")
					+ "  }");
				sep = ", ";
			}
		}
		r = r + "]";
		return r;
	}

	void clear() {
		// Are we locked, here?
		m_name = "";
		clearPluginlist();
		if (m_piname == "") {
			DEBUGPRINT(("No m_piname in pipeline.clear()!?"));
		}
		else {
			m_vizserver->ClearJsonCallbacksWithPrefix(m_piname);
		}
	}

	void FFGLPipeline::CheckAutoload() {

		std::time_t throttle = 1;  // don't check more often than this number of seconds
		std::time_t tm = time(0);
		if ((tm - m_file_lastcheck) < throttle) {
			return;
		}

		std::string fpath = m_filepath;

		struct _stat statbuff;
		int e = _stat(fpath.c_str(), &statbuff);
		if (e != 0) {
			throw NosuchException("Error in _stat fpath=%s - e=%d", fpath.c_str(), e);
		}
		if (statbuff.st_mtime > m_file_lastupdate) {

			DEBUGPRINT(("Pipeline file %s was updated!  Reloading!", m_filepath.c_str()));

			// Keep the same sidmin/sidmax and other values
			load(m_piname, m_name, m_filepath,
				m_sidmin, m_sidmax,
				m_pipeline_enabled);
		}
	}

	void load(std::string piname, std::string name, std::string fpath, int sidmin, int sidmax, bool enabled) {
		std::string current_name = m_name;

		DEBUGPRINT(("Pipeline.load name=%s", name.c_str(), fpath.c_str()));
		bool exists = NosuchFileExists(fpath);
		cJSON* json;
		if (!exists) {
			// If it's a pipeline that doesn't exist,
			// make a copy of the current one
			if (NosuchFileExists(m_filepath)) {
				NosuchFileCopy(m_filepath, fpath);
			}
			else {
				throw NosuchException("No such file: fpath=%s", fpath.c_str());
			}
		}
		std::string err;
		json = jsonReadFile(fpath, err);
		if (!json) {
			throw NosuchException(err.c_str());
		}
		loadJson(piname, name, json);
		jsonFree(json);

		struct _stat statbuff;
		int e = _stat(fpath.c_str(), &statbuff);
		if (e != 0) {
			throw NosuchException("Error in _stat fpath=%s - e=%d", fpath.c_str(), e);
		}

		m_name = name;
		m_piname = piname;
		m_filepath = fpath;
		m_file_lastupdate = statbuff.st_mtime;
		m_file_lastcheck = statbuff.st_mtime;
		setSidrange(sidmin, sidmax);

		m_pipeline_enabled = enabled;
		setEnableInput(enabled);
	}

	void loadJson(std::string piname, std::string name, cJSON* json) {

		clear();

		m_name = name;

		cJSON* plugins = jsonGetArray(json, "pipeline");
		if (!plugins) {
			throw NosuchException("No 'pipeline' value in config");
		}

		int nplugins = cJSON_GetArraySize(plugins);
		for (int n = 0; n < nplugins; n++) {
			cJSON *p = cJSON_GetArrayItem(plugins, n);
			NosuchAssert(p);
			if (p->type != cJSON_Object) {
				throw NosuchException("Hey! Item #%d in pipeline isn't an object?", n);
			}

			std::string plugintype = "ffgl";

			// allow plugin as an alias for ffglplugin.
			cJSON* plugin = jsonGetString(p, "plugin");
			if (!plugin) {
				plugin = jsonGetString(p, "ffglplugin");
			}

			if (plugin == NULL) {
				throw NosuchException("Hey! Item #%d in pipeline doesn't specify plugin?", n);
			}

			// If an explicit vtag (note, not viztag) isn't given, use plugin name
			const char* name = plugin->valuestring;
			std::string vtag = NosuchToLower(jsonNeedString(p, "vtag", name));
			std::string viztag = NosuchSnprintf("%s:%s", piname.c_str(), vtag.c_str());

			bool enabled = jsonNeedBool(p, "enabled", true);  // optional, default is 1 (true)
			bool moveable = jsonNeedBool(p, "moveable", true);  // optional, default is 1 (true)
			cJSON* params = jsonGetArray(p, "params");  // optional, params can be NULL

			// Default is to enable the plugin as soon as we added it, but you
			// can add an "enabled" value to change that.

			FFGLPluginInstance* pi = addToPipeline(name, viztag, enabled, params);
			if (!pi) {
				DEBUGPRINT(("Unable to add plugin=%s", name));
				continue;
			}
			pi->setMoveable(moveable);

			DEBUGPRINT1(("Pipeline, loaded plugin=%s viztag=%s", name, viztag.c_str()));

			cJSON* vizletdump = jsonGetArray(p, "vizletdump");
			if (vizletdump != NULL) {
				int nvals = cJSON_GetArraySize(vizletdump);
				for (int n = 0; n < nvals; n++) {
					cJSON *p = cJSON_GetArrayItem(vizletdump, n);
					NosuchAssert(p);
					if (p->type != cJSON_Object) {
						DEBUGPRINT(("non-Object in vizletdump array!?"));
						continue;
					}
					std::string meth = jsonNeedString(p, "method", "");
					cJSON* params = jsonGetObject(p, "params");
					if (!params) {
						throw NosuchException("No params value in vizletdump?");
					}
					DEBUGPRINT1(("Pipeline load meth=%s params=%s", meth.c_str(), cJSON_PrintUnformatted(params)));
					std::string fullmethod = NosuchSnprintf("%s:%s.%s", piname.c_str(), name, meth.c_str());
					const char* s = m_vizserver->ProcessJson(fullmethod.c_str(), params, "12345");
				}
			}

		}
	}

	VizServer* m_vizserver;
	FFGLPluginList m_pluginlist;
	bool m_pipeline_enabled;

	std::string m_piname;
	std::string m_name;

	int m_sidmin;
	int m_sidmax;
	std::string m_spriteparams;		// name of file

	std::string m_filepath;
	std::time_t m_file_lastupdate;		// last mod time of filepath
	std::time_t m_file_lastcheck;		// last time it was checked

	FFGLViewportStruct fboViewport;
	FFGLTextureStruct m_texture;
	FFGLFBO fbo1;
	FFGLFBO fbo2;
	FFGLFBO* fbo_input;
	FFGLFBO* fbo_output;
};

#endif
