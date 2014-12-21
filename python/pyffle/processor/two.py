import pyffle.builtin
from random import random
from OpenGL.GL import *

from pyffle.processor.default import Processor
from pyffle.builtin import debug

def randvertex():
	# Random points on the upper half of the display
	glVertex2f(random()*2.0 - 1.0,random())

class TwoProcessor(Processor):

	def __init__(self):
		Processor.__init__(self,passthru=True)

		debug("TwoProcessor.__init__")

	def draw(self):

		glEnable(GL_BLEND)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

		for i in range(10):
			# glColor4f(random(),random(),random(),0.5)
			# glColor4f(1.0,0.0,0.0,0.5)
			# glColor4f(0.0,1.0,0.0,0.5)
			glColor4f(0.0,0.0,1.0,0.5)
			glBegin(GL_QUADS)
			randvertex()
			randvertex()
			randvertex()
			randvertex()
			randvertex()
			glEnd()

		return ""
