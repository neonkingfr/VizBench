import pygame

from pygame import *
from pygame.locals import *

class MouseEvent:
	def __init__(self,ev,relpos):
		if hasattr(ev,"button"):
			self.button = ev.button
		self.relpos = relpos
		
class NthDisplayList:

	def __init__(self,screen,bgcolor):
		self.list = []
		self.bgcolor = bgcolor
		self.screen = screen
		self.last_mouse_item = None
		self.screen.fill(bgcolor)
		self.selected = None

	def refresh(self):
		self.screen.fill(self.bgcolor)
		l = len(self.list)
		for n in range(l):
			d = self.list[n]
			if d.visible:
				d.paint(self.screen)
				d.dirty = False

	def add(self,d,offset=None,visible=True):
		if offset != None:
			d.move(offset)
		d.visible = visible
		self.list.append(d)

	def select(self,widget,args):
		if self.selected != None:
			self.selected.unselect()
		widget.select()
		self.selected = widget
		self.selected_args = args

	def drawline(self,pt0,pt1,color,width=1):
		pygame.draw.line(self.screen,color,pt0,pt1,width)

	def drawrect(self,rect,color,width=1):
		pygame.draw.rect(self.screen,color,rect,width)

	def drawpoly(self,pts,color,width=1):
		pygame.draw.polygon(self.screen,color,pts,width)

	def paint(self):
		l = len(self.list)
		for n in range(l):
			d = self.list[n]
			if d.visible and d.dirty:
				# print "Painting item n=",n
				d.paint(self.screen)
				d.dirty = False

	def numitems(self):
		return len(self.list)

	def mouse_event(self,ev):

		if ev.type == MOUSEBUTTONUP or ev.type == MOUSEMOTION:
			if self.last_mouse_item != None:
				self.last_mouse_item.mouse_event(ev)
			if ev.type == MOUSEBUTTONUP:
				self.last_mouse_item = None
			return

		if ev.type != MOUSEBUTTONDOWN:
			return

		l = len(self.list)
		for n in range(l):
			d = self.list[n]
			if d.visible and d.mouseable and d.contains(ev.pos):
				d.mouse_event(ev)
				self.last_mouse_item = d
				return

		self.last_mouse_item = None  # not really needed.  Just in case
