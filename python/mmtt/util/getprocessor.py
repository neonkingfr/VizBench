from traceback import format_exc

import mmtt.builtin
import mmtt.processor
from mmtt.util import recompile
from mmtt.builtin import debug

def getprocessor(bname):
	debug("getprocessor in python begins")
	blower = bname.lower()
	bpath = "mmtt.processor."+blower
	if bname == "Default":
		bclassname = "Processor"
	else:
		bclassname = bname + "Processor"
	debug("getprocessor - bname="+bname+" bpath="+bpath)
	try:
		recompile(bpath)
	except:
		debug("getprocessor - unable to recompile: "+bpath+" exception: "+format_exc())
		return None

	try:
		__import__(bpath)
	except:
		debug("getprocessor - unable to import: "+bpath+" exception: "+format_exc())
		return None

	try:
		processormod = getattr(mmtt.processor,blower)
	except:
		debug("getprocessor - no attribute on processor named: "+blower+" exception: "+format_exc())
		return None

	try:
		processorclass = getattr(processormod,bclassname)
	except:
		debug("getprocessor - module "+bpath+" didn't contain a class "+bclassname+" exception: "+format_exc())
		return None
	try:
		bobj = processorclass()
	except:
		debug("getprocessor - unable to instantiate object for module: "+bpath+" class: "+bclassname+" exception: "+format_exc())
		return None
	return bobj
