import mmtt.builtin
from random import random
from OpenGL.GL import *
from OpenGL.GLUT import *
from traceback import format_exc

from Image import open

from mmtt.processor.default import Processor
from mmtt.builtin import *
from mmtt.util import glFreeType
from nosuch.oscutil import *

def randvertex():
	# Random points on the left half of the display
	glVertex2f(-random(),(random()*2.0 - 1.0)/2.0)

def osccallback(ev,d):
	msg = ev.oscmsg
	if len(msg) < 2:
		debug("Not enough values in oscmsg: %s"%str(msg))
		return
	if msg[0] == "/set_text":
		d.set_text(ev.oscmsg[2])
	elif msg[0] == "/set_pos":
		d.set_pos(ev.oscmsg[2],ev.oscmsg[3])
	else:
		debug("Unrecognized osc message: %s" % str(msg))

class TextProcessor(Processor):

	def __init__(self):
		debug("TextProcessor.__init__")
		debug("getpwd=%s" % os.getcwd())
		Processor.__init__(self,passthru=True)
		self.fontname = "poorich.ttf"
		self.fontheight = 80
		self.text = ""
		self.pos = (0,300)
		self.font = glFreeType.font_data(self.fontname,self.fontheight)
		if not self.font:
			debug("Unable to load font %s !!" % self.fontname)
		else:
			debug("Font %s successfully initialized"%self.fontname)

		try:
			oscmon = OscMonitor("127.0.0.1",9876)
			oscmon.setcallback(osccallback,self)
		except:
			debug("Unable to start OscMonitor: %s" % format_exc())

		# self.imageinit("glass.bmp")

	def imageinit(self,filename):
		self.imagefile = publicpath(filename)
		try:
			self.imageID = self.loadImage(self.imagefile)
			debug("imageID = %d" % self.imageID)

		except:
			debug("Unable to read imagefile: %s" % format_exc())

	def setupTexture(self):
		"""Render-time texture environment setup
		This method encapsulates the functions required to set up
		for textured rendering.
		"""
		# texture-mode setup, was global in original
		glEnable(GL_TEXTURE_2D)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
		# glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL)

		# re-select our texture, could use other generated textures
		# if we had generated them earlier...
		glBindTexture(GL_TEXTURE_2D, self.imageID)   # 2d texture (x and y size)

	def __del__(self):
		debug("TextProcessor.__del__")

	def set_text(self,t):
		debug("SETTING TEXT to t=%s" % t)
		self.text = t

	def set_pos(self,x,y):
		debug("SETTING POS to %f , %f" % (x,y))
		self.pos = (x,y)

	def draw(self):

		glEnable(GL_BLEND)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

		pos = self.pos
		# NOTE: the return value of glPrint is the x width of the text
		self.font.glPrint(pos[0],pos[1],self.text)

		# self.setupTexture()
		# glColor4f(1.0,1.0,1.0,0.5)
		# glBegin(GL_QUADS)
		# glTexCoord2f(0.0,0.0); glVertex2f(-0.5,-0.5)
		# glTexCoord2f(128.0,0.0); glVertex2f(0.5,-0.5)
		# glTexCoord2f(128.0,128.0); glVertex2f(0.5,0.5)
		# glTexCoord2f(0.0,128.0); glVertex2f(-0.5,0.5)
		# glTexCoord2f(0.0,0.0); glVertex2f(-0.5,-0.5)
		# glEnd()

		return ""

	def loadImage( self, imageName ):
		"""Load an image file as a 2D texture using PIL

		This method combines all of the functionality required to
		load the image with PIL, convert it to a format compatible
		with PyOpenGL, generate the texture ID, and store the image
		data under that texture ID.

		Note: only the ID is returned, no reference to the image object
		or the string data is stored in user space, the data is only
		present within the OpenGL engine after this call exits.
		"""
		im = open(imageName)
		try:
			ix, iy, image = im.size[0], im.size[1], im.tostring("raw", "RGB", 0, -1)
			debug("Able to use tostring RGB")
		except SystemError:
			debug("Unable to use tostring RGB?")
			return -1
		# generate a texture ID
		ID = glGenTextures(1)
		# make it current
		glBindTexture(GL_TEXTURE_2D, ID)
		glPixelStorei(GL_UNPACK_ALIGNMENT,1)
		# copy the texture into the current texture ID
		debug("TEXTURE ix,iy=%f,%f" % (ix,iy))
		glTexImage2D(GL_TEXTURE_2D, 0, 3, ix, iy, 0, GL_RGB, GL_UNSIGNED_BYTE, image)
		# return the ID for use
		return ID

