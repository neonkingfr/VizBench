#ifndef _EVENT_H
#define _EVENT_H

class PyEvent {
public:
	virtual std::string type() = 0;
	virtual PyObject* python_object() = 0;
};

class ButtonPyEvent : public PyEvent {
public:
	ButtonPyEvent(std::string bn) {
		_bn = bn;
	}
	virtual std::string type() = 0;
	PyObject* python_object() {
		return Py_BuildValue("{s:s,s:s}",
			"type",type().c_str(),
			"region",_bn.c_str());
	}
private:
	std::string _bn;
};

class ButtonDownPyEvent : public ButtonPyEvent {
public:
	ButtonDownPyEvent(std::string bn) : ButtonPyEvent(bn) {}
	std::string type() { return "ButtonDown"; }
};

class ButtonUpPyEvent : public ButtonPyEvent {
public:
	ButtonUpPyEvent(std::string bn) : ButtonPyEvent(bn) {}
	std::string type() { return "ButtonUp"; }
};

class CursorPyEvent : public PyEvent {
public:
	CursorPyEvent(std::string bn, int sidnum, std::string sidsource, double x, double y, double depth) {
		_bn = bn;
		_sidnum = sidnum;
		_sidsource = sidsource;
		_x = x;
		_y = y;
		_depth = depth;
		NosuchDebug(1,"CursorPyEvent, bn=%s xy=%.4f,%.4f depth=%.4f",bn.c_str(),x,y,depth);
	}
	virtual std::string type() = 0;
	PyObject* python_object() {
		std::string sidstr = NosuchSnprintf("%d/%s",_sidnum,_sidsource.c_str());
		return Py_BuildValue("{s:s,s:s,s:s,s:f,s:f,s:f}",
			"type",type().c_str(),
			"region",_bn.c_str(),
			"sid",sidstr.c_str(),
			"x",_x,
			"y",_y,
			"z",_depth);
	}
private:
	std::string _bn;
	int _sidnum;
	std::string _sidsource;
	double _x;
	double _y;
	double _depth;
};

class CursorDownPyEvent : public CursorPyEvent {
public:
	CursorDownPyEvent(std::string bn, int sidnum, std::string sidsource, double x, double y, double depth) : CursorPyEvent(bn,sidnum,sidsource,x,y,depth) { }
	std::string type() { return "CursorDown"; }
};

class CursorDragPyEvent : public CursorPyEvent {
public:
	CursorDragPyEvent(std::string bn, int sidnum, std::string sidsource, double x, double y, double depth) : CursorPyEvent(bn,sidnum,sidsource,x,y,depth) { }
	std::string type() { return "CursorDrag"; }
};

class CursorUpPyEvent : public CursorPyEvent {
public:
	CursorUpPyEvent(std::string bn, int sidnum, std::string sidsource, double x, double y) : CursorPyEvent(bn,sidnum,sidsource,x,y,0.0) { }
	std::string type() { return "CursorUp"; }
};

#endif