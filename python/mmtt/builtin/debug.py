# NOTE: Normally, the debug function is implemented using the
# C code inside the mmtt module that is built into the Mmtt plugin.
# This is just provided so you can test
# things (like loading and importing Processor plugins) outside of Mmtt.

def debug(msg):
	print "Mmtt debug: "+msg
