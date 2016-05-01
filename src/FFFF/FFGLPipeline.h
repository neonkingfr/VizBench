#ifndef _FFGLPipeline_H
#define _FFGLPipeline_H

class FFGLPipeline {
public:
	FFGLPipeline() {
		m_pipeline_enabled = false;
		m_sidmin = 0;
		m_sidmax = MAX_SESSIONID;
	};

	bool do_ffgl_plugin(FFGLPluginInstance* plugin, int which); // which: 0 = first one, 1 = middle, 2 = last, 3 = one and only one, 4 none
	void doPipeline(int window_width, int window_height);
	FFGLPluginInstance* FFGLNewPluginInstance(FFGLPluginDef* plugin, std::string viztag);
	void paintTexture();
	void setSidrange(int sidmin, int sidmax);
	void setEnableInput(bool onoff);

	void clear() {
		DEBUGPRINT1(("FFGLPipeline.clear is called"));
		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
			DEBUGPRINT1(("--- deleteing FFGLPluginInstance *it=%ld", (long)(*it)));
			delete *it;
		}
		m_pluginlist.clear();
	}
	FFGLPluginInstance* find_plugin(std::string viztag) {
		for (FFGLPluginList::iterator it = m_pluginlist.begin(); it != m_pluginlist.end(); it++) {
			FFGLPluginInstance* p = *it;
			if (viztag == p->viztag()) {
				return *it;
			}
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
		for (size_t n = 1; n<sz; n++ ) {
			int n1 = rand() % sz;
			int n2 = rand() % sz;
			if ( n1 != n2 ) {
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
	void moveplugin(std::string viztag,int places) {
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

	FFGLPluginList m_pluginlist;
	bool m_pipeline_enabled;
	std::string m_name;
	int m_sidmin;
	int m_sidmax;

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
