#include "NosuchUtil.h"
#include "Region.h"
#include "Vizlet.h"

#if 0
void Region::setRegionParams(std::string overname, std::string mainname) {
	std::string cachename = overname + "." + mainname;
	RegionParams* sp = vizlet()->findRegionParams(cachename);
	if ( ! sp ) {
		RegionParams* overparams = vizlet()->getRegionParams(overname);
		RegionParams* mainparams = vizlet()->getRegionParams(mainname);
		if ( !overparams || !mainparams ) {
			DEBUGPRINT(("Unable to setRegionParams for %s",cachename.c_str()));
			return;
		}
		sp = new RegionParams();
		sp->applyParamsFrom(overparams);
		sp->applyParamsFrom(mainparams);
		vizlet()->cacheRegionParams(cachename,sp);
	}
	_params = sp;
}

#endif