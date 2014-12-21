#ifndef _PATCH_H
#define _PATCH_H

#include <map>

class Patch {

public:
	Patch(std::string name) {
		_name = name;
	}
	~Patch();

	void setEffects(std::string s) {
		_effects = s;
	}

	std::string getEffects() {
		return _effects;
	}

	void setRegionParamPath(std::string regionname, std::string parampath) {
		_regionParamPath.insert(std::pair<std::string,std::string>(regionname,parampath));
	}

	std::string getRegionParamPath(std::string regionname) {
		auto it = _regionParamPath.find(regionname);
		if ( it != _regionParamPath.end() ) {
			return it->second;
		} else {
			return "";
		}
	}

	std::string name() { return _name; }

private:
	std::string _name;
	std::map<std::string,std::string> _regionParamPath;
	std::string _effects;
};

#endif
