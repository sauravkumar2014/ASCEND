# General-purpose popup window for reporting texty stuff

import gtk, gtk.glade, pango
import ascpy
from varentry import *

class ImageDialog:
	def __init__(self,browser,parent,imagefilename,title):
		self.browser = browser;

		# GUI config
		_xml = gtk.glade.XML(browser.glade_file,"imagedialog")
		self.window = _xml.get_widget("imagedialog")
		self.window.set_title(title)

		if self.browser.icon:
			self.window.set_icon(self.browser.icon)

		self.parent = None
		if parent:
			self.parent = parent
			self.window.set_transient_for(self.parent)

		self.imageview = _xml.get_widget("imageview")
		self.closebutton = _xml.get_widget("closebutton")

		self.imageview.set_from_file(imagefilename)

		_xml.signal_autoconnect(self)

	def on_infodialog_close(self,*args):
		self.window.response(gtk.RESPONSE_CLOSE);

	def run(self):
		self.window.run()
		self.window.hide()
