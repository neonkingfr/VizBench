#ifndef NOSUCHCOLOR_H
#define NOSUCHCOLOR_H

#include "VizDebug.h"
#include "math.h"

class VizColor {
	
public:
	VizColor() {
		setrgb(0,0,0);
	}

	VizColor(int r_, int g_, int b_) {
		setrgb(r_,g_,b_);
	}
	
	VizColor(double h, double l, double s) {
		h = fmod( h , 360.0);
		sethls(h,l,s);
	}

	void setrgb(int r,int g,int b) {
		m_red = r;
		m_green = g;
		m_blue = b;
		_ToHLS();
	}

	void sethls(double h,double l,double s) {
		if ( h > 360.0 ) {
			DEBUGPRINT(("Hey, hue in VizColor > 360?"));
		}
		m_hue = h;
		m_luminance = l;
		m_saturation = s;
		_ToRGB();
	}
	
	int r() { return m_red; }
	int g() { return m_green; }
	int b() { return m_blue; }
	double hue() { return m_hue; }
	double luminance() { return m_luminance; }
	double saturation() { return m_saturation; }

	int _min(int a, int b) {
		if ( a < b )
			return a;
		else
			return b;
	};

	int _max(int a, int b) {
		if ( a > b )
			return a;
		else
			return b;
	};

	void _ToHLS() {
		double minval = _min(m_red, _min(m_green, m_blue));
		double maxval = _max(m_red, _max(m_green, m_blue));
		double mdiff = maxval-minval;
		double msum = maxval + minval;
		m_luminance = msum / 510;
		if ( maxval == minval ) {
			m_saturation = 0;
			m_hue = 0;
		} else {
			double rnorm = (maxval - m_red) / mdiff;
			double gnorm = (maxval - m_green) / mdiff;
			double bnorm = (maxval - m_blue) / mdiff;
			if ( m_luminance <= .5 ) {
				m_saturation = mdiff/msum;
			} else {
				m_saturation = mdiff / (510 - msum);
			}
			// _saturation = (_luminance <= .5) ? (mdiff/msum) : (mdiff / (510 - msum));
			if ( m_red == maxval ) {
				m_hue = 60 * (6 + bnorm - gnorm);
			} else if ( m_green == maxval ) {
				m_hue = 60 * (2 + rnorm - bnorm);
			} else if ( m_blue == maxval ) {
				m_hue = 60 * (4 + gnorm - rnorm);
			}
			m_hue = fmod(m_hue, 360.0);
		}
	}

	void _ToRGB() {
		if ( m_saturation == 0 ) {
			m_red = m_green = m_blue = (int)(m_luminance * 255);
		} else {
			double rm2;
			if ( m_luminance <= 0.5 ) {
				rm2 = m_luminance + m_luminance * m_saturation;
			} else {
				rm2 = m_luminance + m_saturation - m_luminance * m_saturation;
			}
			double rm1 = 2 * m_luminance - rm2;
			m_red = _ToRGB1(rm1, rm2, m_hue + 120);
			m_green = _ToRGB1(rm1, rm2, m_hue);
			m_blue = _ToRGB1(rm1, rm2, m_hue - 120);
		}
	}

	int _ToRGB1(double rm1, double rm2, double rh) {
		if ( rh > 360 ) {
			rh -= 360;
		} else if ( rh < 0 ) {
			rh += 360;
		}

		if ( rh < 60 ) {
			rm1 = rm1 + (rm2 - rm1) * rh / 60;
		} else if ( rh < 180 ) {
			rm1 = rm2;
		} else if ( rh < 240 ) {
			rm1 = rm1 + (rm2 - rm1) * (240 - rh) / 60;
		}
		return (int)(rm1 * 255);
		
	}

private:
	int m_red;  // 0 to 255
	int m_green;  // 0 to 255
	int m_blue;  // 0 to 255
	double m_hue;   // 0.0 to 360.0
	double m_luminance;  // 0.0 to 1.0
	double m_saturation;  // 0.0 to 1.0

};

#endif
