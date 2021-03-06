import gtk
import pango
import ascpy

from varentry import *
from properties import *
from unitsdialog import *
from study import *

BROWSER_FIXED_COLOR = "#008800"
BROWSER_FREE_COLOR = "#000088"
BROWSER_SETTING_COLOR = "#4444AA"

BROWSER_INCLUDED_COLOR = "black"
BROWSER_UNINCLUDED_COLOR = "#888888"

class ModelView:
	def __init__(self,browser,builder):
		self.browser = browser # the parent object: the entire ASCEND browser

		self.builder = builder
		self.notes = browser.library.getAnnotationDatabase()	

		self.modelview = builder.get_object("browserview")
		
		# name, type, value, foreground, weight, editable, status-icon
		columns = [str,str,str,str,int,bool,gtk.gdk.Pixbuf]

		self.otank = {}

		# name, type, value, foreground, weight, editable, status-icon
		columns = [str,str,str,str,int,bool,gtk.gdk.Pixbuf]
		self.modelstore = gtk.TreeStore(*columns)
		titles = ["Name","Type","Value"];
		self.modelview.set_model(self.modelstore)
		self.tvcolumns = [ gtk.TreeViewColumn() for _type in columns[:len(titles)] ]
		
		self.modelview.connect("row-expanded", self.row_expanded )
		self.modelview.connect("button-press-event", self.on_treeview_event )
		self.modelview.connect("key-release-event",self.on_treeview_event )

		# data columns are: name type value colour weight editable
		
		i = 0
		for tvcolumn in self.tvcolumns[:len(titles)]:
			tvcolumn.set_title(titles[i])
			self.modelview.append_column(tvcolumn)			

			if(i==2):
				# add status icon
				renderer1 = gtk.CellRendererPixbuf()
				tvcolumn.pack_start(renderer1, False)
				tvcolumn.add_attribute(renderer1, 'pixbuf', 6)

			renderer = gtk.CellRendererText()
			tvcolumn.pack_start(renderer, True)
			tvcolumn.add_attribute(renderer, 'text', i)
			tvcolumn.add_attribute(renderer, 'foreground', 3)
			tvcolumn.add_attribute(renderer, 'weight', 4)
			if(i==2):
				tvcolumn.add_attribute(renderer, 'editable', 5)
				renderer.connect('edited',self.cell_edited_callback)
			i = i + 1

		#--------------------
		# get all menu icons and set up the context menu for fixing/freeing vars
		_imagelist = []
		for i in range(6):
			_imagelist.append("image%s" % (i+1))
		self.browser.builder.add_objects_from_file(self.browser.glade_file, _imagelist)
		self.browser.builder.add_objects_from_file(self.browser.glade_file, ["treecontext"])
		
		self.treecontext = self.browser.builder.get_object("treecontext")
		self.fixmenuitem = self.browser.builder.get_object("fix1")
		self.freemenuitem = self.browser.builder.get_object("free1")
		self.propsmenuitem = self.browser.builder.get_object("properties1")
		self.observemenuitem = self.browser.builder.get_object("observe1")
		self.studymenuitem = self.browser.builder.get_object("study1")
		self.unitsmenuitem = self.browser.builder.get_object("units1")
		
		self.fixmenuitem.connect("activate",self.fix_activate)
		self.freemenuitem.connect("activate",self.free_activate)
		self.propsmenuitem.connect("activate",self.props_activate)
		self.observemenuitem.connect("activate",self.observe_activate)
		self.studymenuitem.connect("activate", self.study_activate)
		self.unitsmenuitem.connect("activate",self.units_activate)

		if not self.treecontext:
			raise RuntimeError("Couldn't create browsercontext")

	def setSimulation(self,sim):
		# instance hierarchy
		self.sim = sim
		self.modelstore.clear()
		self.otank = {} # map path -> (name,value)
		self.browser.disable_menu()
		try:
			self.make( self.sim.getName(),self.sim.getModel() )
			self.browser.enable_on_model_tree_build()
		except Exception,e:
			self.browser.reporter.reportError("Error building tree: %s" % e);
		self.browser.maintabs.set_current_page(1);

	def clear(self):
		self.modelstore.clear()
		self.otank = {}

#   --------------------------------------------
#   INSTANCE TREE

	def get_tree_row_data(self,instance): # for instance browser
		_value = str(instance.getValue())
		_type = str(instance.getType())
		_name = str(instance.getName())
		_fgcolor = BROWSER_INCLUDED_COLOR
		_fontweight = pango.WEIGHT_NORMAL
		_editable = False
		_statusicon = None
		if instance.getType().isRefinedSolverVar():
			_editable = True
			_fontweight = pango.WEIGHT_BOLD
			if instance.isFixed():
				_fgcolor = BROWSER_FIXED_COLOR
			else:
				_fgcolor = BROWSER_FREE_COLOR
				_fontweight = pango.WEIGHT_BOLD
			_status = instance.getStatus();
			_statusicon = self.browser.statusicons[_status]
		elif instance.isRelation():
			_status = instance.getStatus();
			_statusicon = self.browser.statusicons[_status]
			if not instance.isIncluded():
				_fgcolor = BROWSER_UNINCLUDED_COLOR
		elif instance.isBool() or instance.isReal() or instance.isInt():
			# TODO can't edit constants that have already been refined
			_editable = True
			_fgcolor = BROWSER_SETTING_COLOR
			_fontweight = pango.WEIGHT_BOLD
		elif instance.isSymbol() and not instance.isConst():
			_editable = True
			_fgcolor = BROWSER_SETTING_COLOR
			_fontweight = pango.WEIGHT_BOLD
			
		#if(len(_value) > 80):
		#	_value = _value[:80] + "..."
		
		return [_name, _type, _value, _fgcolor, _fontweight, _editable, _statusicon]

	def make_row( self, piter, name, value ): # for instance browser
		assert(value)
		_piter = self.modelstore.append( piter, self.get_tree_row_data(value) )
		return _piter

	def refreshtree(self):
		# @TODO FIXME use a better system than colour literals!
		for _path in self.otank: # { path : (name,value) }
			_iter = self.modelstore.get_iter(_path)
			_name, _instance = self.otank[_path]
			self.modelstore.set_value(_iter, 2, _instance.getValue())
			if _instance.getType().isRefinedSolverVar():
				if _instance.isFixed() and self.modelstore.get_value(_iter,3)==BROWSER_FREE_COLOR:
					self.modelstore.set_value(_iter,3,BROWSER_FIXED_COLOR)
				elif not _instance.isFixed() and self.modelstore.get_value(_iter,3)==BROWSER_FIXED_COLOR:
					self.modelstore.set_value(_iter,3,BROWSER_FREE_COLOR)
				self.modelstore.set_value(_iter, 6, self.browser.statusicons[_instance.getStatus()])
			elif _instance.isRelation():
				self.modelstore.set_value(_iter, 6, self.browser.statusicons[_instance.getStatus()])
				if _instance.isIncluded():
					self.modelstore.set_value(_iter,3,BROWSER_INCLUDED_COLOR)
				else:
					self.modelstore.set_value(_iter,3,BROWSER_UNINCLUDED_COLOR)

	def get_selected_type(self):
		return self.get_selected_instance().getType()

	def get_selected_instance(self):
		model,iter = self.modelview.get_selection().get_selected()
		if iter is None:
			return None
		path = model.get_path(iter)
		name,instance = self.otank[path]
		return instance
	
	def cell_edited_callback(self, renderer, path, newtext, **kwargs):
		# get back the Instance object we just edited (having to use this seems like a bug)
		path = tuple( map(int,path.split(":")) )

		if not self.otank.has_key(path):
			raise RuntimeError("cell_edited_callback: invalid path '%s'" % path)
			return

		_name, _instance = self.otank[path]

		if _instance.isReal():
			if _instance.getValue() == newtext:
				return
			# only real-valued things can have units
			
			_e = RealAtomEntry(_instance,newtext);
			try:
				_e.checkEntry()
				_e.setValue()
				_e.exportPreferredUnits(self.browser.prefs)
			except InputError, e:
				self.browser.reporter.reportError(str(e))
				return;

		else:
			if _instance.isBool():
				_lower = newtext.lower();
				if _lower.startswith("t") or _lower.startswith("y") or _lower.strip()=="1":
					newtext = 1
				elif _lower.startswith("f") or _lower.startswith("n") or _lower.strip()=="0":
					newtext = 0
				else:
					self.browser.reporter.reportError("Invalid entry for a boolean variable: '%s'" % newtext)
					return
				_val = bool(newtext);
				if _val == _instance.getValue():
					self.browser.reporter.reportNote("Boolean atom '%s' was not altered" % _instance.getName())
					return
				_instance.setBoolValue(_val)

			elif _instance.isInt():
				_val = int(newtext)
				if _val == _instance.getValue():
					self.browser.reporter.reportNote("Integer atom '%s' was not altered" % _instance.getName())
					return
				_instance.setIntValue(_val)
			elif _instance.isSymbol():
				_val = str(newtext)
				if _val == _instance.getValue():
					self.browser.reporter.reportNote("Symbol atom '%s' was not altered" % _instance.getName())
					return
				_instance.setSymbolValue(ascpy.SymChar(_val))
						
			else:
				self.browser.reporter.reportError("Attempt to set a non-real, non-boolean, non-integer value!")
				return

		# now that the variable is set, update the GUI and re-solve if desired
		_iter = self.modelstore.get_iter(path)
		self.modelstore.set_value(_iter,2,_instance.getValue())

		if _instance.getType().isRefinedSolverVar():
			self.modelstore.set_value(_iter,3,BROWSER_FIXED_COLOR) # set the row green as fixed
		
		self.browser.do_solve_if_auto()

	def make_children(self, value, piter ):
		assert(value)
		if value.isCompound():
			children=value.getChildren();
			for child in children:
				try:
					_name = child.getName();
					_piter = self.make_row(piter,_name,child)
					_path = self.modelstore.get_path(_piter)
					self.otank[_path]=(_name,child)
					#self.browser.reporter.reportError("2 Added %s at path %s" % (_name,repr(_path)))
				except Exception,e:
					self.browser.reporter.reportError("%s: %s" % (_name,e))
	
	def make(self, name=None, value=None, path=None, depth=1):
		if path is None:
			# make root node
			piter = self.make_row( None, name, value )
			path = self.modelstore.get_path( piter )
			self.otank[ path ] = (name, value)
			#self.browser.reporter.reportError("4 Added %s at path %s" % (name, path))
		else:
			name, value = self.otank[ path ]

		assert(value)

		piter = self.modelstore.get_iter( path )
		if not self.modelstore.iter_has_child( piter ):
			#self.browser.reporter.reportNote( "name=%s has CHILDREN..." % name )
			self.make_children(value,piter)

		if depth:
		    for i in range( self.modelstore.iter_n_children( piter ) ):
		        self.make( path = path+(i,), depth = depth - 1 )
		else:
			self.modelview.expand_row("0",False)

	def row_expanded( self, modelview, piter, path ):
		self.make( path = path )


#   ------------------------------
#   CONTEXT MENU

	def on_treeview_event(self,widget,event):
		_path = None
		_contextmenu = False
		if event.type==gtk.gdk.KEY_RELEASE:
			_keyval = gtk.gdk.keyval_name(event.keyval)
			_path, _col = self.modelview.get_cursor()
			if _keyval=='Menu':
				_contextmenu = True
				_button = 3
			elif _keyval == 'F2':
				print "F2 pressed"
				self.modelview.set_cursor(_path,self.tvcolumns[2],1)
												
				return
		elif event.type==gtk.gdk.BUTTON_PRESS:
			_x = int(event.x)
			_y = int(event.y)
			_button = event.button
			_pthinfo = self.modelview.get_path_at_pos(_x, _y)
			if _pthinfo is not None:
				_path, _col, _cellx, _celly = _pthinfo
				if event.button == 3:
					_contextmenu = True

		if _path:
			_name,_instance = self.otank[_path]
			# set the statusbar
			nn = self.notes.getNotes(self.sim.getModel().getType(),ascpy.SymChar("inline"),_name)
			for n in nn:
				print "%s: (%s) %s" % (n.getId(),str(n.getLanguage()),n.getText())
		
			self.builder.get_object("free_variable").set_sensitive(False)
			self.builder.get_object("fix_variable").set_sensitive(False)
			self.builder.get_object("propsmenuitem").set_sensitive(False)
			if _instance.isReal():
				self.builder.get_object("units").set_sensitive(True)
			if _instance.getType().isRefinedSolverVar():
				self.builder.get_object("propsmenuitem").set_sensitive(True)
				if _instance.isFixed():
					self.builder.get_object("free_variable").set_sensitive(True)
				else:
					self.builder.get_object("fix_variable").set_sensitive(True)
			elif _instance.isRelation():
				self.builder.get_object("propsmenuitem").set_sensitive(True)

		if not _contextmenu:
			#print "NOT DOING ANYTHING ABOUT %s" % gtk.gdk.keyval_name(event.keyval)
			return 

		_canpop = False;
		# self.browser.reporter.reportError("Right click on %s" % self.otank[_path][0])

		self.unitsmenuitem.set_sensitive(False)
		self.fixmenuitem.set_sensitive(False)
		self.freemenuitem.set_sensitive(False)
		self.observemenuitem.set_sensitive(False)
		self.studymenuitem.set_sensitive(False)
		self.propsmenuitem.set_sensitive(False)					

		if _instance.isReal():
			print "CAN POP: real atom"
			_canpop = True
			self.unitsmenuitem.set_sensitive(True)

		if _instance.getType().isRefinedSolverVar():
			_canpop = True
			self.propsmenuitem.set_sensitive(True)
			self.observemenuitem.set_sensitive(True)
			if _instance.isFixed():
				self.freemenuitem.set_sensitive(True)
				if len(self.browser.observers) > 0:
					self.studymenuitem.set_sensitive(True)
			else:
				self.fixmenuitem.set_sensitive(True)
		elif _instance.isRelation():
			_canpop = True
			self.propsmenuitem.set_sensitive(True)					
		elif _instance.isModel():
			# MODEL instances have a special context menu:
			_menu = self.get_model_context_menu(_instance)
			self.modelview.grab_focus()
			self.modelview.set_cursor(_path,_col,0)
			print "RUNNING POPUP MENU"
			_menu.popup(None,None,None,_button,event.time)
			return

		if not _canpop:
			return 

		self.modelview.grab_focus()
		self.modelview.set_cursor( _path, _col, 0)
		self.treecontext.popup( None, None, None, _button, event.time)
		return 1

	def get_model_context_menu(self,instance):
		menu = gtk.Menu()
		
		if instance.isPlottable():
			print "PLOTTABLE"
			mi = gtk.ImageMenuItem("P_lot",True);
			img = gtk.Image()
			img.set_from_file(self.browser.options.assets_dir+'/plot.png')
			mi.set_image(img)
			mi.show()
			mi.connect("activate",self.plot_activate)
			menu.append(mi);
			sep = gtk.SeparatorMenuItem(); sep.show()
			menu.append(sep)
		
		mi = gtk.ImageMenuItem("Run method...",False)
		mi.set_sensitive(False)
		img = gtk.Image()
		img.set_from_stock(gtk.STOCK_EXECUTE,gtk.ICON_SIZE_MENU)
		mi.set_image(img)
		mi.show()
		menu.append(mi)

		sep = gtk.SeparatorMenuItem(); sep.show()
		menu.append(sep)

		t = instance.getType()
		ml = t.getMethods()
		if len(ml):
			for m in ml:
				mi = gtk.MenuItem(m.getName(),False)
				mi.show()
				mi.connect("activate",self.run_activate,instance,m)
				menu.append(mi)		
		
		return menu

	def run_activate(self,widget,instance,method):
		print "RUNNING %s" % method.getName()
		try:
			self.browser.sim.run(method,instance)
		except Exception,e:
			self.browser.reporter.reportError(str(e))
		self.refreshtree()		

	def fix_activate(self,widget):
		_path,_col = self.modelview.get_cursor()
		_name, _instance = self.otank[_path]
		self.set_fixed(_instance,True);
		_instance.setFixed(True)
		return 1

	def free_activate(self,widget):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		self.set_fixed(_instance,False)
		return 1

	def plot_activate(self,widget):
	
		self.browser.reporter.reportNote("plot_activate...");
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		if not _instance.isPlottable():
			self.browser.reporter.reportError("Can't plot instance %s" % _instance.getName().toString())
			return
		else:
			self.browser.reporter.reportNote("Instance %s about to be plotted..." % _instance.getName().toString())

		print("Plotting instance '%s'..." % _instance.getName().toString())

		_plot = _instance.getPlot()

		print "Title: ", _plot.getTitle()
		_plot.show(True)

		return 1

	def props_activate(self,widget,*args):
		if not hasattr(self,'sim'):
			self.browser.reporter.reportError("Can't show properties until a simulation has been created.");
			return
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		if _instance.isRelation():
			print "Relation '"+_instance.getName().toString()+"':", \
				_instance.getRelationAsString(self.sim.getModel())
			_dia = RelPropsWin(self.browser,_instance);
			_dia.run();
		elif _instance.getType().isRefinedSolverVar():
			_dia = VarPropsWin(self.browser,_instance);
			_dia.run();
		else:
			self.browser.reporter.reportWarning("Select a variable or relation first...")

	def observe_activate(self,widget,*args):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		if _instance.getType().isRefinedSolverVar():
			print "OBSERVING",_instance.getName().toString()
			self.browser.observe(_instance)

	def on_fix_variable_activate(self,*args):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		self.set_fixed(_instance,True)

	def on_free_variable_activate(self,*args):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		self.set_fixed(_instance,False)

	def set_fixed(self,instance,val):
		if instance.getType().isRefinedSolverVar():
			f = instance.isFixed();
			if (f and not val) or (not f and val):
				instance.setFixed(val)
				self.browser.do_solve_if_auto()


	def study_activate(self, *args):
		_path,_col = self.modelview.get_cursor()
		_instance = self.otank[_path][1]
		self.browser.observe(_instance)
		_dia = StudyWin(self.browser,_instance);
		_dia.run()

	def units_activate(self,*args):
		T = self.get_selected_type()
		try:
			_un = UnitsDialog(self.browser,T)
			_un.run()
		except:
			self.browser.reporter.reportError("Unable to display units dialog.")

