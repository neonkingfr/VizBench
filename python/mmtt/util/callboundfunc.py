from traceback import format_exc

import mmtt.builtin
from mmtt.builtin import debug

def callboundfunc(f):
	# debug("Hi from callboundfunc!")
	typename = f.__class__.__name__
	if typename != "function" and typename != "builtin_function_or_method" and typename != "instancemethod":
		debug("callboundfunc given non-function")
		return "callboundfunc was given a non-function? typename="+typename
	# debug("callboundfunc about to call f")
	msg = ""
	try:
		msg = f()
	except:
		debug("callboundfunc except!")
		msg = format_exc()
	# debug("callboundfunc msg="+msg)
	return msg
