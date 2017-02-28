#ifndef NOSUCHGRAPHICS_H
#define NOSUCHGRAPHICS_H

#include <math.h>
#include <float.h>

#include "VizColor.h"

// class Sprite;

#define RADIAN2DEGREE(r) ((r) * 360.0 / (2.0 * (double)M_PI))
#define DEGREE2RADIAN(d) (((d) * 2.0 * (double)M_PI) / 360.0 )

// #define CHECK_VECTOR
#ifdef CHECK_VECTOR_SANITY
void checkVectorSanity(VizVector v) {
	if ( _isnan(v.x) || _isnan(v.y) ) {
		DEBUGPRINT(("checkVectorSanity found NaN!"));
	}
	if ( v.x > 10.0 || v.y > 10.0 ) {
		DEBUGPRINT(("checkVectorSanity found > 10.0!"));
	}
}
#else
#define checkVectorSanity(v)
#endif

class VizPos {
public:
	VizPos() {
		set(0.0,0.0,0.0);
	}
	VizPos(double xx, double yy, double zz) {
		set(xx,yy,zz);
	};
	void set(double xx, double yy, double zz) {
		x = xx;
		y = yy;
		z = zz;
	}
	double mag() {
		return sqrt( (x*x) + (y*y) + (z*z) );
	}
	VizPos normalize() {
		double leng = mag();
		return VizPos(x/leng, y/leng, z/leng);
	}
	VizPos add(VizPos p) {
		return VizPos(x+p.x,y+p.y,z+p.z);
	}
	VizPos mult(double m) {
		return VizPos(x*m,y*m,z*m);
	}
	VizPos sub(VizPos v) {
		return VizPos(x-v.x,y-v.y,z-v.z);
	}
	double heading() {
        return -atan2(-y, x);
	}
	bool isequal(VizPos p) {
		return ( x == p.x && y == p.y && z == p.z );
	}

	double x;
	double y;
	double z;
};

class VizVector {
public:
	VizVector() {
		// set(FLT_MAX,FLT_MAX,FLT_MAX);
		set(0.0,0.0);
	}
	VizVector(VizPos p) {
		x = p.x;
		y = p.y;
	}
	VizVector(double xx, double yy) {
		set(xx,yy);
	};
	void set(double xx, double yy) {
		x = xx;
		y = yy;
	}
	bool isnull() {
		return ( x == FLT_MAX && y == FLT_MAX );
	}
	bool isequal(VizVector p) {
		return ( x == p.x && y == p.y );
	}
	VizVector sub(VizVector v) {
		return VizVector(x-v.x,y-v.y);
	}
	VizVector add(VizVector v) {
		return VizVector(x+v.x,y+v.y);
	}
	double mag() {
		return sqrt( (x*x) + (y*y) );
	}
	VizVector normalize() {
		double leng = mag();
		return VizVector(x/leng, y/leng);
	}
	VizVector mult(double m) {
		return VizVector(x*m,y*m);
	}
	VizVector rotate(double radians, VizVector about = VizVector(0.0f,0.0f) ) {
		double c, s;
		c = cos(radians);
		s = sin(radians);
		x -= about.x;
		y -= about.y;
		double newx = x * c - y * s;
		double newy = x * s + y * c;
		newx += about.x;
		newy += about.y;
		return VizVector(newx,newy);
	}
	double heading() {
        return -atan2(-y, x);
	}

	double x;
	double y;
};

typedef struct PointMem {
	float x;
	float y;
	float z;
} PointMem;

class VizGraphics {
public:
	VizGraphics();

	bool m_filled;
	VizColor m_fill_color;
	double m_fill_alpha;
	bool m_stroked;
	VizColor m_stroke_color;
	double m_stroke_alpha;

	void fill(VizColor c, double alpha);
	void noFill();
	void stroke(VizColor c, double alpha);
	void noStroke();
	void strokeWeight(double w);
	void background(int);
	void rect(double x, double y, double width, double height);
	void pushMatrix();
	void popMatrix();
	void translate(double x, double y);
	void scale(double x, double y);
	void rotate(double degrees);
	void line(double x0, double y0, double x1, double y1);
	void triangle(double x0, double y0, double x1, double y1, double x2, double y2, int xdir, int ydir);
	void quad(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3);
	void ellipse(double x0, double y0, double w, double h);
	void polygon(PointMem* p, int npoints, int xdir, int ydir);
};

#endif
