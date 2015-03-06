import pygame

from pygame import *
from pygame.locals import *

from nthdisplay import MouseEvent

default_bgcolor = Color("white")

class NthDrawable:

	def __init__(self,rect,color=None,bgcolor=None):
		self.rect = rect
		self.dirty = True
		if color:
			self.color = color
		else:
			self.color = Color("black")
		if bgcolor:
			self.bgcolor = bgcolor
		else:
			global default_bgcolor
			self.bgcolor = default_bgcolor
		self.origbgcolor = self.bgcolor
		self.highcolor = Color("red")
		self.selectcolor = Color(255,100,100)
		self.visible = True
		self.selected = False
		self.mouseable = True
		self.callback_up = None
		self.callback_up_data = None
		self.callback_down_data = None
		self.callback_down = None
		self.callback_motion_data = None
		self.callback_motion = None
		self.lastpos = (0,0)
		self.sendmotion = False
		self.is_down = False

	@staticmethod
	def set_default_bgcolor(bg):
		global default_bgcolor
		default_bgcolor = bg

	def contains(self,point):
		return self.rect.collidepoint(point)

	def move(self,offset):
		self.set_rect(self.rect.move(offset))
		self.dirty = True

	def set_rect(self,rect):
		self.rect = rect

	def lefttop(self):
		return (self.rect.left,self.rect.top)

	def set_color(self,color):
		self.color = color
		self.dirty = True

	def set_bgcolor(self,bgcolor):
		self.bgcolor = self.origbgcolor = bgcolor
		self.dirty = True

	def select(self):
		# print "Setting select in widget=",self
		self.selected = True
		self.bgcolor = self.selectcolor
		self.dirty = True

	def unselect(self):
		# print "UNSetting select in widget=",self
		self.selected = False
		self.bgcolor = self.origbgcolor
		self.dirty = True

	def set_callback(self,cb,data):
		self.callback_down = cb
		self.callback_down_data = data
		# print "SETTING CALLBACK_down=",cb," data=",data

	def set_callback_up(self,cb,data,strict=False):
		self.callback_up = cb
		self.callback_up_data = data
		self.callback_up_strict = strict

	def set_callback_motion(self,cb,data):
		self.callback_motion = cb
		self.callback_motion_data = data

	def mouse_event(self,ev):
		relpos = (float(ev.pos[0] - self.rect.topleft[0])/self.rect.width,
				float(ev.pos[1] - self.rect.topleft[1])/self.rect.height)
		mev = MouseEvent(ev,relpos)
		if ev.type == MOUSEBUTTONDOWN:
			if self.callback_down and not self.is_down:
				# print "CALLING callback_down=",self.callback_down," data=",self.callback_down_data," button=",ev.button
				self.callback_down(self.callback_down_data,mev)
			self.lastpos = ev.pos
			self.sendmotion = False
			self.is_down = True
			# self.bgcolor = self.highcolor
			# self.dirty = True
		elif ev.type == MOUSEBUTTONUP:
			if self.callback_up:
				if self.is_down:
					if not self.callback_up_strict or self.contains(ev.pos):
						self.callback_up(self.callback_up_data)
						self.is_down = False
			else:
				self.is_down = False
			if self.selected:
				self.bgcolor = self.selectcolor
				self.dirty = True
			else:
				self.bgcolor = self.origbgcolor
				self.dirty = True
		elif ev.type == MOUSEMOTION:
			if self.callback_motion:
				dpos = (ev.pos[0]-self.lastpos[0],
					ev.pos[1]-self.lastpos[1])
				# Yes, I know this isn't the real distance
				dist = abs(dpos[0])+abs(dpos[1])
				if self.sendmotion == False and dist > 12:
					self.sendmotion = True
				if self.sendmotion:
					self.callback_motion(self.callback_motion_data,dpos)
					self.lastpos = ev.pos


class NthTextable(NthDrawable):

	def __init__(self,rect,text,fontsize=72,
			color=None,bgcolor=None,border=True):
		NthDrawable.__init__(self,rect,color=color,bgcolor=bgcolor)
		self.mouseable = False
		self.textrect = self.rect
		if rect.width < 2 or rect.height < 2:
			print "NthTextable rect is too small! (",rect,")"
			return
		self.border = border
		self.set_text(text)
		self.font = pygame.font.Font(None,fontsize)

	def do_text(self,text,textrect,surface):
		fsz = self.font.size(text)
		if fsz[0] == 0:
			# nothing to render?
			return
		textrender = self.font.render(text, 1, self.color)
		scalewidth = textrect.width - 2
		scaleheight = textrect.height - 2
		scalewidth = int(textrect.height * (float(fsz[0])/fsz[1]))
		indent = (textrect.width - scalewidth) / 2

		if indent < 0:
			scalewidth = textrect.width
			textrender = pygame.transform.smoothscale(
				textrender, (scalewidth,scaleheight))
			surface.blit(textrender,(1,1))
		else:
			textrender = pygame.transform.smoothscale(
				textrender, (scalewidth,scaleheight))
			surface.blit(textrender,(1+indent,1))

	def set_text(self,text):
		if type(text) != type("string"):
			print "HEY, text isn't string !?  text=",text," type(text)=",type(text)," type(string)=",type("string")
			self.text = "NONSTRING"
			raise Exception, "set_text got nonstring?"
		else:
			self.text = text
		self.dirty = True

	def set_rect(self,rect):
		NthDrawable.set_rect(self,rect)
		self.textrect = rect

class NthRect(NthDrawable):

	def __init__(self,rect):
		NthDrawable.__init__(self,rect)
		self.mouseable = False

	def paint(self,screen):
		pygame.draw.rect(screen,self.color,self.rect,1)

class NthFilledRect(NthDrawable):

	def __init__(self,rect):
		NthDrawable.__init__(self,rect)

	def paint(self,screen):
		pygame.draw.rect(screen,self.color,self.rect,0)

class NthTextArrowDown(NthTextable):
	def __init__(self,rect,text,fontsize=72,
			color=None,bgcolor=None,border=True):
		NthTextable.__init__(self,rect,text,color=color,bgcolor=bgcolor,border=border)
		self.mouseable = True

	def paint(self,screen):
		self.surface = pygame.Surface(self.textrect.size)
		self.surface.fill(self.bgcolor)
		if self.border:
			pts = [ self.rect.topleft, self.trirect.topleft,
				self.trirect.midbottom, self.trirect.topright,
				(self.rect.left+self.rect.width,self.rect.top)
				]
			pygame.draw.polygon(screen,self.bgcolor,pts,0)
			pygame.draw.polygon(screen,self.color,pts,1)

		self.do_text(self.text,self.textrect,self.surface)
		screen.blit(self.surface,self.textrect.topleft)

	def set_rect(self,rect):
		NthTextable.set_rect(self,rect)
		self.textrect = Rect((rect.left+1,rect.top+1),
				(rect.width-2,rect.height/2))
		self.trirect = Rect((rect.left,rect.top+self.textrect.height),
				(rect.width,rect.height-self.textrect.height))


class NthTextArrowLeft(NthTextable):
	def __init__(self,rect,text,fontsize=72,
			color=None,bgcolor=None,border=True):
		NthTextable.__init__(self,rect,text,color=color,bgcolor=bgcolor,border=border)
		self.mouseable = True
		self.awidth = 20

	def paint(self,screen):
		self.surface = pygame.Surface(self.textrect.size)
		self.surface.fill(self.bgcolor)
		if self.border:
			topright = (self.rect.topright[0],self.rect.topright[1])
			bottomright = (self.rect.bottomright[0],self.rect.bottomright[1])
			pts = [
				self.trirect.topright, topright,
				bottomright, self.trirect.bottomright,
				self.trirect.midleft
				]
			pygame.draw.polygon(screen,self.bgcolor,pts,0)
			pygame.draw.polygon(screen,self.color,pts,1)

		self.do_text(self.text,self.textrect,self.surface)
		screen.blit(self.surface,self.textrect.topleft)

	def set_rect(self,rect):
		NthTextable.set_rect(self,rect)
		self.textrect = Rect((rect.left+self.awidth,rect.top+1),
				(rect.width-self.awidth-1,rect.height-2))
		self.trirect = Rect((rect.left,rect.top),
				(self.awidth,rect.height))
		# print("ArrowLeft rect=",self.rect)
		# print("ArrowLeft textrect=",self.textrect)
		# print("ArrowLeft trirect=",self.trirect)



class NthText(NthTextable):

	def __init__(self,rect,text,fontsize=72,
			color=None,bgcolor=None,border=True):
		NthTextable.__init__(self,rect,text,color=color,bgcolor=bgcolor,border=border)
		if rect.width < 2 or rect.height < 2:
			print "NthText rect is too small! (",rect,")"
			return
		self.mouseable = False

	def paint(self,screen):
		self.surface = pygame.Surface(self.rect.size)
		self.surface.fill(self.bgcolor)
		if self.border:
			pygame.draw.rect(self.surface,
				self.color,
				Rect(0,0,self.rect.width,self.rect.height),
				1)

		self.do_text(self.text,self.textrect,self.surface)
		screen.blit(self.surface,self.rect.topleft)
		return

	def set_rect(self,rect):
		NthDrawable.set_rect(self,rect)
		self.textrect = Rect((rect.left+1,rect.top+1),
				(rect.width,rect.height))


class NthButton(NthText):
	def __init__(self,rect,text,color=None,bgcolor=None):
		NthText.__init__(self,rect,text,
			color=color,bgcolor=bgcolor,border=True)
		self.mouseable = True
