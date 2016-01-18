#ifndef _SCALE_H
#define _SCALE_H

#include <string>
#include <vector>
#include <map>

class Scale {
public:
	Scale();
	Scale(std::string nm, int n1, ...);
	static void initialize();
	static bool initialized;
	int closestTo(int pitch);
	static std::vector<std::string> scaleTypes;
	static std::map<std::string,Scale> Scales;

private:
	void clear();
	std::string _name;
	bool _has_note[128];
};

#endif
