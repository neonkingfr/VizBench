#include "PaletteAll.h"

bool Cursor::initialized = false;
// std::vector<std::string> Cursor::behaviourTypes;

Cursor::Cursor(Palette* palette_, int sidnum, std::string sidsource, Region* region_, NosuchVector pos_, double depth_, double area_) {
	_area = area_;
	_palette = palette_;
	_last_pitches.clear();
	_last_channel = -1;
	_last_click = -1;
	_sidsource = sidsource;
	_sidnum = sidnum;
	_region = region_;
	_outline_mem = NULL;

	NosuchDebug(1,"NEW Cursor, region=%d sid=%d/%s",_region,_sidnum,_sidsource.c_str());

	curr_pos = pos_;
	_target_pos = pos_;
	_last_pos = pos_;

	curr_speed = 0.0f;

	// _prev_pos = pos_;
	_previous_musicpos = pos_;
	_last_depth = depth_;
	curr_depth = depth_;
	_target_depth = depth_;
	NosuchDebug(2,"Cursor CONSTRUCTOR sid=%d/%s  pos_ = %.3f %.3f",_sidnum,_sidsource.c_str(),pos_.x,pos_.y);
	_start_time = Palette::now;
	_last_tm = _start_time;
	_last_instantiate = 0;

	// _g_last_smoothedpos = pos_;
	// _g_smoothedpos = pos_;
	// _g_smoothedpos = pos_;

	_g_smoothed_distance = 0.0f;
	curr_degrees = 0.0f;
	_smooth_degrees_factor = 0.2f;
	_target_degrees = 0.0f;
	_g_firstdir = true;
	_touched = 0;
}

void Cursor::initialize() {

	if ( initialized )
		return;
	initialized = true;

	// behaviourTypes.push_back("instantiate"); // instantiate sprites continuously
	// behaviourTypes.push_back("move");        // instantiate and then move a single sprite
	// behaviourTypes.push_back("accumulate");  // continuously accumulate points for a single sprite
}

Cursor::~Cursor() {
	NosuchDebug(1,"CURSOR DESTRUCTOR!!  _sid=%d/%s",_sidnum,_sidsource.c_str());
}

Cursor*
Cursor::clone() {
	Cursor* nc = new Cursor(_palette,_sidnum,_sidsource,_region,curr_pos,curr_depth,_area);
	return nc;
}

double
normalize_degrees(double d) {
	if ( d < 0.0f ) {
		d += 360.0f;
	} else if ( d > 360.0f ) {
		d -= 360.0f;
	}
	return d;
}

void
Cursor::advanceTo(int tm) {

	int dt = tm - _last_tm;
	if ( dt <= 0 ) {
		return;
	}

	// If _pos and _g_smoothedpos are the same (x and y, not z), then there's nothing to smooth
	if ( curr_pos.x == _target_pos.x && curr_pos.y == _target_pos.y ) {
		NosuchDebug(1,"Cursor::advanceTo, current and target are the same");
		return;
	}

	NosuchVector dpos = _target_pos.sub(curr_pos);
	double raw_distance = dpos.mag();
	if ( raw_distance > 1.0f ) {
		NosuchDebug("Cursor::advanceTo, raw_distance>1.0 !?");
		return;
	}
	if ( raw_distance == 0.0f ) {
		NosuchDebug("Cursor::advanceTo, raw_distance=0.0 !?");
		return;
	}

	double this_speed = 1000.0f * raw_distance / dt;
	double speed_limit = 60.0f;
	if ( this_speed > speed_limit ) {
		NosuchDebug("Speed LIMIT (%f) EXCEEDED, throttled to %f",this_speed,speed_limit);
		this_speed = speed_limit;
	}
	// NosuchDebug("=== Cursor::advanceTo tm=%d dt=%d rawdist=%.4f speed=%.5f %s",tm,dt,raw_distance,this_speed,DebugBrief().c_str());

	dpos = dpos.normalize();

	double sfactor = 0.02f;
	// speed it up a bit when the distance gets larger
	if ( raw_distance > 0.4f ) {
		sfactor = 0.1f;
	} else if ( raw_distance > 0.2f ) {
		sfactor = 0.05f;
	}
	this_speed = this_speed * sfactor;

	double dspeed = this_speed - curr_speed;
	// NosuchDebug("    current_speed=%.4f this_speed=%.4f dspeed=%.4f ",curr_speed,this_speed,dspeed);
	double smooth_speed_factor = 0.1f;
	curr_speed = curr_speed + dspeed * smooth_speed_factor;
	dpos = dpos.mult(raw_distance * curr_speed);
	_last_pos = curr_pos;
	curr_pos = curr_pos.add(dpos);
	if ( curr_pos.x > 1.0f ) {
		NosuchDebug("Cursor::advanceTo, x>1.0 !?");
	}
	if ( curr_pos.y > 1.0f ) {
		NosuchDebug("Cursor::advanceTo, y>1.0 !?");
	}

	NosuchVector finaldpos = curr_pos.sub(_last_pos);
	double final_distance = finaldpos.mag();
	// NosuchDebug("    new current_speed=%.4f dpos mult=%.5f final dist=%.5f",curr_speed,raw_distance*curr_speed,final_distance);

	/////////////// smooth the depth
	double depthsmoothfactor = 0.3f;
	double smoothdepth = curr_depth + ((target_depth()-curr_depth)*depthsmoothfactor);

	// NosuchDebug("  curr_depth=%.4f target=%.4f smoothed=%.4f",curr_depth,target_depth(),smoothdepth);

	curr_depth = smoothdepth;

	/////////////// smooth the degrees
	double tooshort = 0.01f; // 0.05f;
	if (raw_distance < tooshort) {
		// NosuchDebug("   raw_distance=%.3f too small %s\n",
		// 	raw_distance,DebugString().c_str());
	} else {
		NosuchVector dp = curr_pos.sub(_last_pos);
		double heading = dp.heading();
		_target_degrees = radian2degree(heading);
		_target_degrees += 90.0;
		_target_degrees = normalize_degrees(_target_degrees);

		// if (_target_degrees < 0.0) {
		// 	NosuchDebug("    ADDING 360! dir was %f",_target_degrees);
		// 	_target_degrees += 360.0;
		// }
		if (_g_firstdir) {
			curr_degrees = _target_degrees;
			// NosuchDebug("firstdir current_degrees = %f",curr_degrees);
			// NosuchDebug("    FIRSTDIR!  curr_degrees = %.4f",curr_degrees);
			_g_firstdir = false;
		} else {
			double dd1 = _target_degrees - curr_degrees;
			double dd;
			if ( dd1 > 0.0f ) {
				if ( dd1 > 180.0f ) {
					dd = -(360.0f - dd1);
					// NosuchDebug("   A dd1=%.4f dd=%.3f forward=%d",dd1,dd,forward);
				}
				else {
					dd = dd1;
					// NosuchDebug("   B dd1=%.4f dd=%.3f forward=%d",dd1,dd,forward);
				}
			} else {
				if ( dd1 < -180.0f ) {
					dd = dd1 + 360.0f;
					// NosuchDebug("   C dd1=%.4f dd=%.3f forward=%d",dd1,dd,forward);
				}
				else {
					dd = dd1;
					// NosuchDebug("   D dd1=%.4f dd=%.3f forward=%d",dd1,dd,forward);
				}
			}
			curr_degrees = curr_degrees + (dd*_smooth_degrees_factor);

			// NosuchDebug("   E curr_deg = %.4f",curr_degrees);
			curr_degrees = normalize_degrees(curr_degrees);
			// NosuchDebug("heading=%.4lf  target_degrees=%.4lf curr_degrees=%.4lf",heading,_target_degrees,curr_degrees);
			// NosuchDebug("other current_degrees = %f",curr_degrees);

			// curr_degrees = curr_degrees + (_target_degrees - curr_degrees) * _smooth_degrees_factor;
			// NosuchDebug("    updated curr_degrees to %.3f",curr_degrees);
		}
		// NosuchDebug("   FINAL current_degrees =%.4f",curr_degrees);
	}
	_last_tm = Palette::now;

	// NosuchDebug("   end of advanceTo %s",DebugString().c_str());
}