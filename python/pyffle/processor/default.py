import time
import sys
import inspect
import os
import pyffle.builtin
from random import randint,random

from pyffle.builtin import debug
from OpenGL.GL import *

from traceback import format_exc

def randvertex():
	pyffle.glVertex2f(random()*2.0 - 1.0,random()*2.0 - 1.0)
	# glVertex2f(random()*2.0 - 1.0,random()*2.0 - 1.0)

class Processor:

	def __init__(self,passthru=True):
		debug("Processor.__init__ time=%f" % (time.time()) )
		self.passthru = passthru

	def processOpenGL(self):
		# debug("processOpenGL")
		self.draw_start()
		self.draw()
		self.draw_finish()

	def draw(self):
		# plugins (classes that extend Processor) should define this
		pass

	def draw_start(self):
		if self.passthru:
			glEnable(GL_TEXTURE_2D);

			# We assume the width,height of the texture is 1.0,1.0
			# This may not always be true - the GetMaxGLTexCoords
			# function in the C code probably needs to be
			# exposed to python.
			w = 1.0
			h = 1.0

			# draw a quad with the input texture mapped onto it

			glBegin(GL_QUADS)
			# lower left
			glTexCoord2d(0.0, 0.0)
			glVertex2f(-1,-1)
			# upper left
			glTexCoord2d(0.0, h)
			glVertex2f(-1,1)
			# upper right
			glTexCoord2d(w, h)
			glVertex2f(1,1)
			# lower right
			glTexCoord2d(w, 0.0)
			glVertex2f(1,-1)
			glEnd()

			glBindTexture(GL_TEXTURE_2D, 0)

			glDisable(GL_TEXTURE_2D)

		# glEnable(GL_BLEND)
		# glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

		return ""

	def draw_finish(self):
		pass

class DefaultProcessor(Processor):

	def __init__(self):
		Processor.__init__(self,passthru=True)

	def draw(self):
		pass
