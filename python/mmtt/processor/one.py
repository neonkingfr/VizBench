import mmtt.builtin
from random import random
from OpenGL.GL import *

from mmtt.processor.default import Processor
from mmtt.builtin import debug

def randvertex():
	# Random points on the left half of the display
	glVertex2f(-random(),(random()*2.0 - 1.0)/2.0)

class OneProcessor(Processor):

	def __init__(self):
		debug("OneProcessor.__init__");
		Processor.__init__(self,passthru=True)

	def draw(self):

		glEnable(GL_BLEND)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

		for i in range(5):
			glColor4f(1.0,1.0,1.0,0.5)
			glBegin(GL_QUADS)
			randvertex()
			randvertex()
			randvertex()
			randvertex()
			randvertex()
			glEnd()

		return ""
