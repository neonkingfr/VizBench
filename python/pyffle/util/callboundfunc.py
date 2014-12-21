from traceback import format_exc

def callboundfunc(f):
	typename = f.__class__.__name__
	if typename != "function" and typename != "builtin_function_or_method" and typename != "instancemethod":
		return "callboundfunc was given a non-function? typename="+typename
	try:
		return f()
	except:
		return format_exc()
