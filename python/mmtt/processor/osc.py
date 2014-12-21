import mmtt.builtin
from nosuch.oscutil import *
from random import random
from OpenGL.GL import *

from mmtt.processor.default import Processor
from mmtt.builtin import debug

# NOTE: although this works, it reveals a current bug in Mmtt - if you
# use this plugin, then unload it and reload it, it will be unable to
# re-bind to the same port.  This can and should be fixed, eventually.

def randvertex():
	# Random points on the upper half of the display
	glVertex2f(random()*2.0 - 1.0,random())

def osccallback(ev,d):
	debug("oscmsg="+str(ev.oscmsg))

class OscProcessor(Processor):

	def __init__(self):
		Processor.__init__(self,passthru=True)

		# This uses Tim Thompson's oscutil library
		oscmon = OscMonitor("127.0.0.1",9876)
		oscmon.setcallback(osccallback,"")

		debug("OscProcessor.__init__")

	def __del__(self):
		debug("OscProcessor.__del__")

	def draw(self):

		glEnable(GL_BLEND)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

		for i in range(10):
			glColor4f(random(),random(),random(),0.5)
			glBegin(GL_QUADS)
			randvertex()
			randvertex()
			randvertex()
			randvertex()
			randvertex()
			glEnd()

		return ""
