# NOTE: Normally, the debug function is implemented using the
# C code inside the pyffle module that is built into the Pyffle plugin.
# This is just provided so you can test
# things (like loading and importing Processor plugins) outside of Pyffle.

def debug(msg):
	print "Pyffle debug: "+msg
