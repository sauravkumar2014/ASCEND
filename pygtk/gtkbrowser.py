try:
	import loading
	#loading.print_status("Loading PSYCO")
	#try:
	#	import psyco
	#	psyco.full()
	#	print "Running with PSYCO optimisation..."
	#except ImportError:
	#	pass

	loading.print_status("Loading python standard libraries")

	import pygtk 
	pygtk.require('2.0') 
	import gtk
	import gtkexcepthook

	import re
	import urlparse
	import optparse
	import platform
	import sys
	import os

	if platform.system() != "Windows":
		try:
			import dl
			_dlflags = dl.RTLD_GLOBAL|dl.RTLD_NOW
		except:
			# On platforms that unilaterally refuse to provide the 'dl' module
			# we'll just set the value and see if it works.
			loading.print_status("Setting dlopen flags","Python 'dl' module not available on this system")
			_dlflags = 258
		# This sets the flags for dlopen used by python so that the symbols in the
		# ascend library are made available to libraries dlopened within ASCEND:
		sys.setdlopenflags(_dlflags)

	loading.print_status("Loading LIBASCEND/ascpy")
	import ascpy

	loading.print_status("Loading PyGTK, pango")

	import pango

	loading.load_matplotlib()

	loading.print_status("Loading ASCEND python modules")
	from preferences import *      # loading/saving of .ini options
	from solverparameters import * # 'solver parameters' window
	from help import *             # viewing help files
	from incidencematrix import *  # incidence/sparsity matrix matplotlib window
	from imagewindow import *      # image viewer window
	from observer import *         # observer tab support
	from properties import *       # solver_var properties dialog
	from varentry import *         # for inputting of variables with units
	from diagnose import * 	       # for diagnosing block non-convergence
	from solverreporter import *   # solver status reporting
	from moduleview import *       # module browser
	from modelview import *        # model browser
	from integrator import *       # integrator dialog	
	from infodialog import *       # general-purpose textual information dialog
	from versioncheck import *     # version check (contacts ascend.cruncher2.dyndns.org)
	from unitsdialog import *      # general-purpose textual information dialog
	from solverhooks import *      # solver hooks for use from Python layer
	import config

	#loading.complete();

except RuntimeError, e:
	print "ASCEND had problems starting up. Please report the following"
	print "error message on ASCEND bug tracker."
	print "\n\nFull error message:",str(e)
	print "\n\nPress ENTER to close this window."
	sys.stdout.flush()
	sys.stdin.readline();
	sys.exit();

except ImportError, e:
	print "\n\n------------------  ERROR  ---------------------"
	print     "ASCEND had problems importing required Python modules."
	print "\nPlease ensure you have all the runtime prerequisites installed."
	print "Please then report a bug if you continue to have problems."
	print "\nFull error message:",str(e)
	if platform.system()=="Windows":
		print "\nYou will also need to report the contents of any popup error"
		print "messages from Windows if any were shown."
	print "\n\nPress ENTER to close this window."
	sys.stdout.flush()
	sys.stdin.readline();
	sys.exit();

loading.print_status("Starting GUI")

# This is my first ever GUI code so please be nice :)
# But I *have* at least read 
# http://www.joelonsoftware.com/uibook/chapters/fog0000000057.html
# and leafed through
# http://developer.gnome.org/projects/gup/hig/

# The fancy tree-view gizmo is the GtkTreeView object. See the article
# http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/300304
# for the original source code on which my implementation was based.

ESCAPE_KEY = 65307

HELP_ROOT = None

#======================================
# Browser is the main ASCEND library/model browser window

class Browser:

#   ---------------------------------
#   SETUP

	def __init__(self,librarypath=None,assetspath=None):

		if assetspath==None:
			if platform.system()=="Windows":
				assetspath=os.path.normpath(os.path.join(os.path.dirname(__file__),"..","glade")) 
			else:
				assetspath=config.PYGTK_ASSETS

		#--------
		# load the file referenced in the command line, if any

		loading.print_status("Parsing options","CONFIG = %s"%config.VERSION)

		#print "Command-line options:",sys.argv
		
		parser = optparse.OptionParser(usage="%prog [[-m typename] file]", version="gtkbrowser $rev$" )
		# add options here if we want

		#print "About to parse..."

		if platform.system() == "Darwin":
			parser.add_option("-p", "--silly-mac-thing"
				,action="store", type="string", dest="process_number"
				,help="Launch Services for Mac passes in a -psn-NNNNN argument that we need to swallow.")		

		parser.add_option("-m", "--model"
			,action="store", type="string", dest="model"
			,help="specify the model to instantiate upon loading modules")		

		parser.add_option("--pygtk-assets"
			,action="store", type="string", dest="assets_dir"
			,help="override the configuration value for the location of assets"\
				+" required by PyGTK for the ASCEND GUI, optional"
			,default=assetspath
		)

		parser.add_option("--library"
			,action="store", type="string", dest="library_path"
			,help="override the configuration value for the library path"
			,default=librarypath
		)

		parser.add_option("--no-auto-sim"
			,action="store_false", dest="auto_sim"
			,help="disable auto-instantiation of MODEL named as the file stem"
			,default=True
		)

		parser.add_option("-t", "--test"
			,action="store", type="string", dest="test"
			,help="load a model and run contained tests without GUI")		

		(self.options, args) = parser.parse_args()

		if len(args)>=1:
			if os.path.isfile(args[0])==False:
				error = '\033[91mERROR : %s is not a file\033[0m'%args[0]
				print error
				sys.exit()
		
		#print "OPTIONS_______________:",self.options

		self.assets_dir = self.options.assets_dir
		#loading.create_window(self.assets_dir)
		
		self.observers = []
		self.currentobservertab = None
		self.clip = None

		#--------
		# load up the preferences ini file

		loading.print_status("Loading preferences")

		self.prefs = Preferences()
		_prefpath = self.prefs.getStringPref("Directories","librarypath",None)
		_preffileopenpath = self.prefs.getStringPref("Directories","fileopenpath",None)
		self.filename = None
	
		#--------
		# set up library path and the path to use for File->Open dialogs
		
		if self.options.library_path != None:
			_path = os.path.abspath(self.options.library_path)
			_pathsrc = "command line options"
			# when a special path is specified, use the last path component as the file-open location
			if platform.system()=="Windows":
				self.fileopenpath = _path.split(":").pop()
			else:
				self.fileopenpath = _path.split(":").pop()
		else:
			if _prefpath:
				_path = _prefpath
				_pathsrc = "user preferences"
			else:
				# default setting, but override with Windows registry if present
				_path = config.LIBRARY_PATH
				_pathsrc = "default (config.py)"

				if platform.system()=="Windows":
					# use the registry
					try:
						import _winreg
						x=_winreg.ConnectRegistry(None,_winreg.HKEY_LOCAL_MACHINE)
						y= _winreg.OpenKey(x,r"SOFTWARE\ASCEND")
						_regpath,t = _winreg.QueryValueEx(y,"ASCENDLIBRARY")
						_winreg.CloseKey(y)
						_winreg.CloseKey(x)
						_path = _regpath						
						os.environ['ASCENDLIBRARY'] = _regpath
						_pathsrc = "Windows registry"
					except:
						# otherwise keep using the default
						pass
			
			if _preffileopenpath:
				self.fileopenpath = _preffileopenpath
			else:
				self.fileopenpath = _path
					
		#--------
		# Create the ASCXX 'Library' object
		
		loading.print_status("Creating ASCEND 'Library' object","ASCENDLIBRARY = "+_path+" FROM "+_pathsrc)
		self.library = ascpy.Library(str(_path))

		self.sim = None
	
		#--------
		# report absence of solvers if nec.

		if not len(ascpy.getSolvers()):
			print "NO SOLVERS LOADED!"
			self.reporter.reportError( "No solvers were loaded! ASCEND is probably not configured correctly." )

		#--test option
		if self.options.test:
			print '================================================================================'
			print 'IN TEST'
			self.test()
			return

		#-------------------
		# Set up the window and main widget actions
		self.glade_file = os.path.join(self.assets_dir,config.GLADE_FILE)

		loading.print_status("Setting up windows") #,"GLADE_FILE = %s" % self.glade_file)

		builder = gtk.Builder()
		#builder.add_from_file(self.glade_file)
		builder.add_objects_from_file(self.glade_file,["integ_icon","browserwin","list_of_td"])
		self.builder=builder
		self.window=self.builder.get_object ("browserwin")

		self.disable_menu()
		self.disable_on_first_run()
		if not self.window:
			raise RuntimeError("Couldn't load window from glade file")

		_display = self.window.get_screen().get_display().get_name()
		_geom=self.prefs.getGeometrySizePosition(_display,"browserwin")
		if _geom:
			self.window.resize(_geom[0],_geom[1])
			self.window.move(_geom[2],_geom[3])
		
		self.window.connect("delete_event", self.delete_event)

		self.browserpaned=self.builder.get_object ("browserpaned")
		_geom2=self.prefs.getGeometryValue(_display,"browserpaned")
		if _geom2:
			self.browserpaned.set_position(_geom2)

		buttons = ["open","reload","solve","integrate","check","methodrun"]
		for n in buttons:
			name = "%sbutton"%n
			setattr(self,name,self.builder.get_object(name))
			getattr(self,name).connect("clicked",getattr(self,"%s_click"%n))

		widgets = ["autotoggle","automenu","methodsel","maintabs","lowertabs","consolescroll","statusbar","browsermenu","reloadwarn"]
		for n in widgets:
			setattr(self,n,self.builder.get_object(n))

		self.autotoggle.connect("toggled",self.auto_toggle)

		self.show_solving_popup=self.builder.get_object("show_solving_popup")
		self.show_solving_popup.set_active(self.prefs.getBoolPref("SolverReporter","show_popup",True))
		self.close_on_converged=self.builder.get_object("close_on_converged")
		self.close_on_converged.set_active(self.prefs.getBoolPref("SolverReporter","close_on_converged",True))
		self.close_on_nonconverged=self.builder.get_object("close_on_nonconverged")
		self.close_on_nonconverged.set_active(self.prefs.getBoolPref("SolverReporter","close_on_nonconverged",True))
		self.solver_engine=self.builder.get_object("solver_engine")
		self.recent_files=self.builder.get_object("recentfiles")

		self.use_relation_sharing=self.builder.get_object("use_relation_sharing")
		self.use_relation_sharing.set_active(self.prefs.getBoolPref("Compiler","use_relation_sharing",True))

		self.use_binary_compilation=self.builder.get_object("use_binary_compilation")
		self.use_binary_compilation.set_active(self.prefs.getBoolPref("Compiler","use_binary_compilation",False))
		self.use_binary_compilation.set_sensitive(self.use_relation_sharing.get_active())
		
		self.check_weekly=self.builder.get_object("check_weekly")
		self.check_weekly.set_active(not(self.prefs.getBoolPref("Browser","disable_auto_check_for_updates",False)))

		self.builder.connect_signals(self)

		#-------
		# Status icons

		self.fixedimg = gtk.Image()
		_fixedimgpath = os.path.join(self.options.assets_dir,'locked.png')

		# this stuff catches some strange environment-variable related problems on Mac OSX.
		try:
			if not os.path.exists(_fixedimgpath):
				raise RuntimeError("Image file '%s' could not be found" % _fixedimgpath)
			_fixedpixbuf = gtk.gdk.pixbuf_new_from_file(_fixedimgpath)
			self.fixedimg.set_from_pixbuf(_fixedpixbuf)
		except Exception,e:
			raise RuntimeError("Failed to load pixbuf '%s' (%s)" % (_fixedimgpath, str(e)))

		self.inactiveimg = gtk.Image()
		self.inactiveimg.set_from_file(os.path.join(self.options.assets_dir,'unattached.png'))

		self.iconstatusunknown = None
		self.iconfixed = self.fixedimg.get_pixbuf()
		self.iconsolved = self.window.render_icon(gtk.STOCK_YES,gtk.ICON_SIZE_MENU)
		self.iconactive = self.window.render_icon(gtk.STOCK_NO,gtk.ICON_SIZE_MENU)
		self.iconinactive = self.inactiveimg.get_pixbuf()
		self.iconunsolved = None

		self.statusicons={
			ascpy.ASCXX_INST_STATUS_UNKNOWN: self.iconstatusunknown
			,ascpy.ASCXX_VAR_FIXED: self.iconfixed
			,ascpy.ASCXX_VAR_SOLVED: self.iconsolved
			,ascpy.ASCXX_VAR_ACTIVE: self.iconactive
			,ascpy.ASCXX_VAR_UNSOLVED: self.iconunsolved
			,ascpy.ASCXX_REL_INACTIVE: self.iconinactive
		}


		self.statusmessages={
			ascpy.ASCXX_INST_STATUS_UNKNOWN: "Status unknown"
			,ascpy.ASCXX_VAR_FIXED: "Fixed"
			,ascpy.ASCXX_VAR_SOLVED: "Converged"
			,ascpy.ASCXX_VAR_ACTIVE: "Active (unconverged)"
			,ascpy.ASCXX_VAR_UNSOLVED: "Not yet visited"
			,ascpy.ASCXX_REL_INACTIVE: "Inactive"
		}

		#-------------------
		# waitwin

		self.waitwin = gtk.gdk.Window(self.window.window,
			gtk.gdk.screen_width(),
			gtk.gdk.screen_height(),
			gtk.gdk.WINDOW_CHILD,
			0,
			gtk.gdk.INPUT_ONLY)

		_cursor = gtk.gdk.Cursor(gtk.gdk.WATCH)
		self.waitwin.set_cursor(_cursor)

		#-------------------
		# pixbufs to be used in the error listing

		self.iconok = self.window.render_icon(gtk.STOCK_YES,gtk.ICON_SIZE_MENU)
		self.iconinfo = self.window.render_icon(gtk.STOCK_DIALOG_INFO,gtk.ICON_SIZE_MENU)
		self.iconwarning = self.window.render_icon(gtk.STOCK_DIALOG_WARNING,gtk.ICON_SIZE_MENU)
		self.iconerror = self.window.render_icon(gtk.STOCK_DIALOG_ERROR,gtk.ICON_SIZE_MENU)

		#--------------------
		# pixbufs for solver_var status

		#--------------------
		# set up the error view

		self.errorview = self.builder.get_object("errorview")
		errstorecolstypes = [gtk.gdk.Pixbuf,str,str,str,int]
		self.errorstore = gtk.TreeStore(*errstorecolstypes)
		errtitles = ["","Location","Message"];
		self.errorview.set_model(self.errorstore)
		self.errcols = [ gtk.TreeViewColumn() for _type in errstorecolstypes]

		i = 0
		for tvcolumn in self.errcols[:len(errtitles)]:
			tvcolumn.set_title(errtitles[i])
			self.errorview.append_column(tvcolumn)			

			if i>0:
				_renderer = gtk.CellRendererText()
				tvcolumn.pack_start(_renderer, True)				
				tvcolumn.add_attribute(_renderer, 'text', i)
				if(i==2):
					tvcolumn.add_attribute(_renderer, 'foreground', 3)
					tvcolumn.add_attribute(_renderer, 'weight', 4)
			else:
				_renderer1 = gtk.CellRendererPixbuf()
				tvcolumn.pack_start(_renderer1, False)				
				tvcolumn.add_attribute(_renderer1, 'pixbuf', int(0))

			i = i + 1


		#--------------------
		# set up the error reporter callback
		self.reporter = ascpy.getReporter()
		self.reporter.setPythonErrorCallback(self.error_callback)


		#-------
		# Solver engine list

		_slvlist = ascpy.getSolvers()
		self.solver_engine_menu = gtk.Menu()
		self.solver_engine_menu.show()
		self.solver_engine.set_submenu(self.solver_engine_menu)
		self.solver_engine_menu_dict = {}
		_fmi = None
		for _s in _slvlist:
			_mi = gtk.RadioMenuItem(_fmi,_s.getName(),False)
			if _fmi==None:
				_fmi = _mi
			_mi.show()
			_mi.connect('toggled',self.on_select_solver_toggled,_s.getName())
			self.solver_engine_menu.append(_mi)
			self.solver_engine_menu_dict[_s.getName()]=_mi	
		
		#-------
		# Recent file list

		self.recent_file_list = gtk.Menu()
		self.recent_file_list.show()
		self.recent_files.set_submenu(self.recent_file_list)
		
		_max_num = int(self.prefs.getStringPref("recentfiles","max","-1"))
		if _max_num == -1:
			self.prefs.setStringPref("recentfiles","max","10")

		_cur_num = int(self.prefs.getStringPref("recentfiles","cur","-1"))
		if _cur_num >= 0:
			for _i in range(_cur_num):
				_fname = self.prefs.getStringPref("recentfiles","file%s"%(_cur_num -_i - 1),"no recent files")
				_mi = gtk.MenuItem(_fname,False)
				_mi.show()
				_mi.connect("activate",self.on_recent_file_select)
				self.recent_file_list.append(_mi)
		else:
			self.recent_file_list.set_state(gtk.STATE_INSENSITIVE)

		_pref_solver = self.prefs.getStringPref("Solver","engine","QRSlv")
		_mi = self.solver_engine_menu_dict.get(_pref_solver)
		if _mi:
			_mi.set_active(1)

		#--------
		# Assign an icon to the main window

		self.icon = None
		if config.ICON_EXTENSION:
			_iconpath = ""
			try:
				_icon = gtk.Image()
				_iconpath = os.path.join(self.assets_dir,'ascend'+config.ICON_EXTENSION)
				_icon.set_from_file(_iconpath)
				_iconpbuf = _icon.get_pixbuf()
				self.window.set_icon(_iconpbuf)
				self.icon = _iconpbuf
			except Exception, e:
				print "FAILED TO SET APPLICATION ICON PATH '%s': %s" % (_iconpath,str(e))
				self.reporter.reportError("FAILED to set application icon '%s': %s"
					 % (_iconpath,str(e)) 
				)

		#-------------------
		# set up the module view

		self.modtank = {}
		self.moduleview = ModuleView(self, self.builder, self.library)
	
		#--------------------
		# set up the methods combobox

		self.methodstore = gtk.ListStore(str)
		self.methodsel.set_model(self.methodstore)
		_methodrenderer = gtk.CellRendererText()
		self.methodsel.pack_start(_methodrenderer, True)
		self.methodsel.add_attribute(_methodrenderer, 'text',0)

		#--------
		# set up the instance browser view

		self.modelview = ModelView(self, self.builder)

		#--------
		# set up the tabs
		self.tabs = {}
		self.activetab = None # most recent observer tab

		#--------
		# set the state of the 'auto' toggle

		self.is_auto = self.prefs.getBoolPref("Browser","auto_solve",True)
		self.autotoggle.set_active(self.is_auto)
		self.automenu.set_active(self.is_auto)
		
		#--------
		# set the state of the 'warn on reload' toggle

		self.reload_warn = self.prefs.getBoolPref("Browser","warn_on_reload",True)
		self.reloadwarn.set_active(self.reload_warn)
		
		#--------
		# tell libascend about this 'browser' object

		#print dir(ascpy.Registry())
		ascpy.Registry().set("browser",self)

		#--------
		# Set up SolverHooks

		print "PYTHON: SETTING UP SOLVER HOOKS"
		self.solverhooks = SolverHooksPythonBrowser(self)
		ascpy.SolverHooksManager_Instance().setHooks(self.solverhooks)

		#--------
		# options
		if(len(args)==1):
			try:
				self.do_open(args[0])
			except RuntimeError,e:
				self.reporter.reportError(str(e))
				return

			#print "Options: ",self.options

			_model = None
			if self.options.model:
				_model = self.options.model
				print "MODEL: '%s'" % _model
			elif self.options.auto_sim:
				_head, _tail = os.path.split(args[0])
				if(_tail):
					_model, _ext = os.path.splitext(_tail)

			if _model:
				try:
					_t=self.library.findType(_model)
					if not _t.isModel():
						if self.options.auto_sim:
							self.reporter.reportError("Won't auto-instantiate with type '%s': not a MODEL." % _model)
					elif _t.hasParameters():
						if self.options.auto_sim:
							self.reporter.reportError("Won't auto-instantiate MODEL %s: model requires parameters." % _model)
					else:
						try:
							self.do_sim(_t)
							if not self.options.model:
								self.reporter.reportNote("Instantiated self-titled model '%s'" %_model)
						except RuntimeError, e:
							self.reporter.reportError("Failed to create instance of '%s': %s" 
								%(_model, str(e))
							);
				except RuntimeError, e:
					if self.options.model:
						self.reporter.reportError("Unknown model type '%s': %s" 
							%(_model, str(e))
						);		

	
		#--------
		# IPython console, if available

		import console
		console.create_widget(self)

		import locale
		if locale.localeconv()['decimal_point'] != '.':
			self.reporter.reportError(
"""Incompatible locale settings detected. Please set your system local to one which
uses '.' as the decimal point separator, such as en_US or en_AU. Note that failing
to make this change will cause input and output values to be truncated.
For details, see http://ascendbugs.cheme.cmu.edu/view.php?id=337"""
			)

	def run(self):
		if not self.options.test:
			#self.window.show()
			loading.print_status("ASCEND is now running")
			loading.complete()
			if self.prefs.getStringPref('Browser','first_run') == None:
				self.prefs.setStringPref('Browser','first_run',time.time())
			else:
				time_now = time.time()
				first_run_time = float(self.prefs.getStringPref('Browser','first_run'))
				if ((time_now-first_run_time)/(3600*24)) >= 7:
					self.auto_update_check()
			gtk.main()

	def test(self):
		print sys.argv[1]
		print sys.argv[3]
		if len(sys.argv)==4:
			ascpy.test_model(str(sys.argv[1]),str(sys.argv[3]))
		#Call the function at the SWIG API level that runs all the tests and pass to it, the *args
		#ascpy is accessible here


#   ------------------
#   SOLVER LIST

	def set_solver(self,solvername):
		""" this sets the active solver in the GUI, which is the default applied to newly instantiated models """
		self.solver = ascpy.Solver(solvername)
		self.prefs.setStringPref("Solver","engine",solvername)
		self.reporter.reportNote("Set solver engine to '%s'" % solvername)

#   --------------------------------------------
# 	MAJOR GUI COMMANDS

	def on_fix_variable_activate(self,*args):
		if not self.sim:
			self.reporter.reportError("No model selected yet")
			return
		self.modelview.on_fix_variable_activate(*args)

	def on_free_variable_activate(self,*args):
		if not self.sim:
			self.reporter.reportError("No model selected yet")
			return
		self.modelview.on_free_variable_activate(*args)

	def on_select_solver_toggled(self,widget,solvername):
		if widget.get_active():
			self.set_solver(solvername)

	def on_recent_file_select(self,widget):
		if widget:
			#	_msg = gtk.MessageDialog(buttons=gtk.BUTTONS_OK,message_format=filename,flags=gtk.DIALOG_MODAL)
			#	_msg.run()
			self.do_open(widget.get_label())

	def update_recent_files(self,filename):
		if filename:
			_max_num = int(self.prefs.getStringPref("recentfiles","max","10"))
			_cur_num = int(self.prefs.getStringPref("recentfiles","cur","0"))
			_should_change = False
			_temp = None
			if _cur_num < _max_num:
				for _i in range(_cur_num):
					_temp = self.prefs.getStringPref("recentfiles","file%s"%(_i),None)
					if _temp == filename: #already in the list before
						_should_change = True
					if _should_change and _i+1 < _cur_num:
						_temp = self.prefs.getStringPref("recentfiles","file%s"%(_i+1),None)
						self.prefs.setStringPref("recentfiles","file%s"%(_i),_temp)
					if _should_change and _i == _cur_num - 1:
						self.prefs.setStringPref("recentfiles","file%s"%(_i),filename)

				if _should_change == False:
					self.prefs.setStringPref("recentfiles","file%s"%(_cur_num),filename)
					self.prefs.setStringPref("recentfiles","cur",_cur_num+1)
			else:
				for _i in range(_max_num):
					_temp = self.prefs.getStringPref("recentfiles","file%s"%(_i),None)
					if _temp == filename: #already in the list before
						_should_change = True
					if _should_change and _i+1 < _max_num:
						_temp = self.prefs.getStringPref("recentfiles","file%s"%(_i+1),None)
						self.prefs.setStringPref("recentfiles","file%s"%(_i),_temp)
					if _should_change and _i == _max_num - 1:
						self.prefs.setStringPref("recentfiles","file%s"%(_i),filename)

				if _should_change == False: #this is a new file, then remove the oldest one 
					for _i in range(_max_num):
						if _i+1 < _max_num:
							_temp = self.prefs.getStringPref("recentfiles","file%s"%(_i+1),None)
							self.prefs.setStringPref("recentfiles","file%s"%(_i),_temp)
						else:
							self.prefs.setStringPref("recentfiles","file%s"%(_i),filename)

	
	def do_open(self,filename):
		# TODO does the user want to lose their work?
		# TODO do we need to chdir?

		_context = self.statusbar.get_context_id("do_open")

		self.errorstore.clear()
		self.modelview.clear()
		self.currentobservertab = None
		for _obs in self.observers:
			if _obs.alive == False:
				_obs.reloaded = True
			_obs.set_dead()
		# self.library.clear()
		#print "Filename =",filename
		self.statusbar.push(_context,"Loading '"+filename+"'")
		try:
			self.filename = filename
			self.disable_menu()
			self.enable_on_file_open()
			# call the low-level 'load' command...
			self.library.load(filename)
			self.update_recent_files(filename)
		except RuntimeError,e:
			self.statusbar.pop(_context)
			raise

		try:
			self.statusbar.pop(_context)
		except TypeError,e:
			print "For some reason, a type error (context=%s,filename=%s): %s" % (_context,filename,e)

		# Load the current list of modules into self.modules
		self.moduleview.refresh(self.library)

		self.sim = None;
		self.maintabs.set_current_page(0);
	
	# See http://www.daa.com.au/pipermail/pygtk/2005-October/011303.html
	# for details on how the 'wait cursor' is done.
	def start_waiting(self, message):
		self.waitcontext = self.statusbar.get_context_id("waiting")
		self.statusbar.push(self.waitcontext,message)

		if self.waitwin:
			self.waitwin.show()

		while gtk.events_pending():
			gtk.main_iteration()
		
	def stop_waiting(self):
		if self.waitwin:
			self.statusbar.pop(self.waitcontext)
			self.waitwin.hide()
		
	def do_sim(self, type_object):
		self.sim = None;
		# TODO: clear out old simulation first!

		#print "DO_SIM(%s)" % str(type_object.getName())		
		self.start_waiting("Compiling...")

		try:
			_v = self.prefs.getBoolPref("Compiler","use_relation_sharing",True)
			ascpy.getCompiler().setUseRelationSharing(_v)
			print "Relation sharing set to",_v

			_v = self.prefs.getBoolPref("Compiler","use_binary_compilation",False)
			ascpy.getCompiler().setBinaryCompilation(_v)
			print "Binary compilation set to",_v

			self.sim = type_object.getSimulation(str(type_object.getName())+"_sim",False)

			#self.reporter.reportNote("SIMULATION ASSIGNED")
		except RuntimeError, e:
			self.stop_waiting()
			self.reporter.reportError(str(e))
			return

		self.stop_waiting()

		# get method names and load them into the GUI
		self.methodstore.clear()
		_methods = self.sim.getType().getMethods()
		_activemethod = None;
		for _m in _methods:
			_i = self.methodstore.append([_m.getName()])
			if _m.getName()=="on_load":
				self.methodsel.set_active_iter(_i)

		self.modelview.setSimulation(self.sim)

		# run the 'on_load' method
		self.start_waiting("Running default method...")
		try:
			#self.reporter.reportNote("SIMULATION CREATED, RUNNING DEFAULT METHOD NOW...")
			self.sim.runDefaultMethod()
		except RuntimeError, e:
			self.stop_waiting()
			self.reporter.reportError(str(e))
			return			
		self.stop_waiting()

		self.modelview.refreshtree()
	
	def do_solve_if_auto(self):
		if self.is_auto:
			self.sim.checkInstance()
			self.do_solve()
		else:
			try:
				self.sim.processVarStatus()
			except RuntimeError,e:
				self.reporter.reportError(str(e))
			self.modelview.refreshtree()

		self.sync_observers()


	def no_built_system(self):
		""" check that the system is 'built', ready for use by the solver. """

		if not self.sim:
			self.reporter.reportError("No model selected yet")
			return 1

		try:
			self.sim.build()
			self.enable_on_sim_build()
		except RuntimeError,e:
			self.reporter.reportError("Couldn't build system: %s" % str(e));
			return 1
		
		return 0;
	
	def do_solve(self):
		if self.no_built_system():
			return

		if not hasattr(self,'solver'):
			self.reporter.reportError("No solver assigned!")
			return

		self.start_waiting("Solving with %s..." % self.solver.getName())

		if self.prefs.getBoolPref("SolverReporter","show_popup",True):
			reporter = PopupSolverReporter(self,self.sim)
		else:
			reporter = SimpleSolverReporter(self)

		try:
			self.sim.solve(self.solver,reporter)
		except RuntimeError,e:
			self.reporter.reportError(str(e))	

		self.stop_waiting()
		
		self.modelview.refreshtree()

	def do_integrate(self):
		if self.no_built_system():
			return

		try:
			self.sim.build()
		except RuntimeError,e:
			self.reporter.reportError("Couldn't build system: %s",str(e))
			return
		integrator = ascpy.Integrator(self.sim)	
		try:
			integrator.findIndependentVar()
		except RuntimeError,e:
			self.reporter.reportNote(str(e))

		integwin = IntegratorWindow(self,self.sim)		
		_integratorreporter = integwin.run()
		if _integratorreporter!=None:
			_integratorreporter.run()
			self.sim.processVarStatus()
			self.modelview.refreshtree()
		

	def do_check(self):
		if self.no_built_system():
			return

		self.start_waiting("Checking system...")

		try:
			self.sim.checkInstance()
			self.reporter.reportWarning("System instance check run, check above for error (if any).")
			# the above gives output but doesn't throw errors or return a status.
			# ... this is a problem (at the C level)

			status = self.sim.checkDoF()
			if status==ascpy.ASCXX_DOF_UNDERSPECIFIED:
				self.on_show_fixable_variables_activate(None)
			elif status==ascpy.ASCXX_DOF_OVERSPECIFIED:
				self.on_show_freeable_variables_activate(None)
			elif status==ascpy.ASCXX_DOF_STRUCT_SINGULAR:
				if not self.sim.checkStructuralSingularity():
					sing = self.sim.getSingularityInfo()
					title = "Structural singularity"
					text = title
					msgs = {
						"The singularity can be reduced by freeing the following variables" : sing.freeablevars
						,"Relations involved in the structural singularity" : sing.rels
						,"Variables involved in the structural singularity" : sing.vars
					}
					for k,v in msgs.iteritems():
						text+="\n\n%s:" % k
						if len(v):
							_l = [j.getName() for j in v]
							_l.sort()
							text+= "\n\t" + "\n\t".join(_l)
						else:
							text += "\nnone"

					_dialog = InfoDialog(self,self.window,text,title)
					_dialog.run()
			else:
				self.reporter.reportNote("System DoF check OK")

		except RuntimeError, e:
			self.stop_waiting()
			self.reporter.reportError(str(e))
			return

		self.stop_waiting()
		self.modelview.refreshtree()

	def do_method(self,method):

		try:
			self.sim.run(method)
		except RuntimeError,e:
			self.reporter.reportError(str(e))
		
		self.sim.processVarStatus()
		self.modelview.refreshtree()

	def do_quit(self):
		loading.print_status("Saving window location")		
		self.reporter.clearPythonErrorCallback()

		_w,_h = self.window.get_size()
		_t,_l = self.window.get_position()
		_display = self.window.get_screen().get_display().get_name()
		self.prefs.setGeometrySizePosition(_display,"browserwin",_w,_h,_t,_l );

		_p = self.browserpaned.get_position()
		self.prefs.setGeometryValue(_display,"browserpaned",_p);

		loading.print_status("Saving current directory")			
		self.prefs.setStringPref("Directories","fileopenpath",self.fileopenpath)

		self.prefs.setBoolPref("Browser","auto_solve",self.is_auto)

		loading.print_status("Saving preferences")
		# causes prefs to be saved unless they are still being used elsewher
		del(self.prefs)

		loading.print_status("Clearing error callback")		
		self.reporter.clearPythonErrorCallback()

		loading.print_status("Closing down GTK")
		gtk.main_quit()

		loading.print_status("Clearing library")			
		self.library.clear()
		
		loading.print_status("Quitting")

		return False

	def on_tools_sparsity_click(self,*args):

		self.reporter.reportNote("Preparing incidence matrix...")
		try:
			_im = self.sim.getIncidenceMatrix()
			self.reporter.reportNote("Plotting incidence matrix...")
			_sp = IncidenceMatrixWindow(_im)
			_sp.run()
		except RuntimeError,e:
			self.reporter.reportError(str(e))

	def on_units_click(self,*args):
		T = self.modelview.get_selected_type()
		_un = UnitsDialog(self,T)
		_un.run()

	def on_tools_incidencegraph_click(self,*args):
		self.reporter.reportNote("Preparing incidence graph...")
		fname = os.tempnam()
		self.start_waiting("Creating incidence graph...")
		try:
			self.sim.write(fname,'dot') # create a PNG file in f
		except Exception,e:
			self.stop_waiting()
			self.reporter.reportError("Failed to create incidence graph: %s" % str(e))
			return
		self.stop_waiting()
		_ig = ImageWindow(self, self.window, fname, title="Incidence Graph", delete=True)
		_ig.run()
		self.reporter.reportNote("Deleted temporary file")

	def on_tools_repaint_tree_activate(self,*args):
		self.reporter.reportNote("Repainting model view...")
		self.modelview.refreshtree()

	def on_diagnose_blocks_activate(self,*args):
		try:
			_bl = self.sim.getActiveBlock()
			_db = DiagnoseWindow(self,_bl)
			_db.run();
		except RuntimeError, e:
			self.reporter.reportError(str(e))
			return

	def on_add_observer_click(self,*args):
		self.create_observer()

	def on_keep_observed_click(self,*args):
		print "KEEPING..."
		if self.currentobservertab is None:
			self.reporter.reportError("No observers defined for this model!")
			return
		self.tabs[self.currentobservertab].do_add_row()

	def on_copy_observer_matrix_click(self,*args):
		if self.clip == None:
			self.clip = gtk.Clipboard()

		if len(self.observers) <= 0:
			self.reporter.reportError("No observer defined!")
			return
		self.tabs[self.currentpage].copy_to_clipboard(self.clip)

	def on_use_relation_sharing_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("Compiler","use_relation_sharing",_v)
		self.reporter.reportNote("Relation sharing set to "+str(_v))
		self.use_binary_compilation.set_sensitive(_v);

	def on_use_binary_compilation_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("Compiler","use_binary_compilation",_v)
		self.reporter.reportNote("Binary compilation set to "+str(_v))

	def on_show_solving_popup_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("SolverReporter","show_popup",_v)
		
	def on_close_on_converged_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("SolverReporter","close_on_converged",_v)

	def on_close_on_nonconverged_toggle(self,checkmenuitem,*args):
		_v = checkmenuitem.get_active()
		self.prefs.setBoolPref("SolverReporter","close_on_nonconverged",_v)

	def on_show_variables_near_bounds_activate(self,*args):
		_epsilon = 1e-4;
		try:
			_vars = self.sim.getVariablesNearBounds(_epsilon)
		except RuntimeError,e:
			self.reporter.reportError("Unable to show variables near bounds:\n%s"%str(e))
			return
		text = "Variables Near Bounds"
		title=text;
		text += "\n"
		if len(_vars):
			for _v in _vars:
				text += "\n%s"%_v.getName()
		else:
			text +="\nnone"
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_show_vars_far_from_nominals_activate(self,*args):
		_bignum = self.prefs.getRealPref("Browser","far_from_nominals",10);
		try:
			_vars = self.sim.getVariablesFarFromNominals(_bignum)
		except RuntimeError,e:
			self.reporter.reportError("Unable to show variables far from nominals:\n%s"%str(e))
			return
		text = "Variables Far from Nominals"
		title=text;
		text += "\n"
		if len(_vars):
			for _v in _vars:
				text += "\n%s"%_v.getName()
		else:
			text +="\nnone"

		text+="\n\nAbove calculated using a relative error of %f" % float(_bignum)
		text+="\nModify this value in .ascend.ini, section '[Browser]', key 'far_from_nominals'."
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_show_config_activate(self,*args):
		text = "Configuration"
		title=text;
		text += "\n\nEnvironment variables:\n"
		if len(os.environ):
			for _k,_v in sorted(os.environ.items()):
				text += "  %s=%s\n" % (_k,_v)
		else:
			text +="\nempty"

		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()


#   ----------------------------------
#   ERROR PANEL

	def get_error_row_data(self,sev,filename,line,msg):
		try:
			_sevicon = {
				0   : self.iconok
				,1  : self.iconinfo
				,2  : self.iconwarning
				,4  : self.iconerror
				,8  : self.iconinfo
				,16 : self.iconwarning
				,32 : self.iconerror
				,64 : self.iconerror
			}[sev]
		except KeyError:
			_sevicon = self.iconerror

		_fontweight = pango.WEIGHT_NORMAL
		if sev==32 or sev==64:
			_fontweight = pango.WEIGHT_BOLD
		
		_fgcolor = "black"
		if sev==8:
			_fgcolor = "#888800"
		elif sev==16:
			_fgcolor = "#884400"
		elif sev==32 or sev==64:
			_fgcolor = "#880000"
		elif sev==0:
			_fgcolor = BROWSER_FIXED_COLOR
		
		if not filename and not line:
			_fileline = ""
		else:
			if(len(filename) > 25):
				filename = "..."+filename[-22:]
			_fileline = filename + ":" + str(line)

		_res = (_sevicon,_fileline,msg.rstrip(),_fgcolor,_fontweight)
		#print _res
		return _res  

	def error_callback(self,sev,filename,line,msg):
		#print "SEV =",sev
		#print "FILENAME =",filename
		#print "LINE =",line
		#print "MSG =",msg
		pos = self.errorstore.append(None, self.get_error_row_data(sev, filename,line,msg))
		path = self.errorstore.get_path(pos)
		col = self.errorview.get_column(3)
		self.errorview.scroll_to_cell(path,col)		
		return 0;

#   --------------------------------
#   BUTTON METHODS

	def open_click(self,*args):
		#loading.print_status("CURRENT FILEOPENPATH is",self.fileopenpath)
		dialog = gtk.FileChooserDialog("Open ASCEND model...",
			self.window,
			gtk.FILE_CHOOSER_ACTION_OPEN,
			(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK)
		)
		dialog.set_current_folder(self.fileopenpath)
		dialog.set_default_response(gtk.RESPONSE_OK)
		dialog.set_transient_for(self.window)
		dialog.set_modal(True)

		filter = gtk.FileFilter()
		filter.set_name("*.a4c, *.a4l")
		filter.add_pattern("*.[Aa]4[Cc]")
		filter.add_pattern("*.[Aa]4[Ll]")
		dialog.add_filter(filter)

		filter = gtk.FileFilter()
		filter.set_name("All files")
		filter.add_pattern("*")
		dialog.add_filter(filter)

		response = dialog.run()
		_filename = dialog.get_filename()
		print "\nFILENAME SELECTED:",_filename
		
		_path = dialog.get_current_folder()
		if _path:
			self.fileopenpath = _path
		
		dialog.hide()

		if response == gtk.RESPONSE_OK:
			self.reporter.reportNote("File %s selected." % dialog.get_filename() )
			self.library.clear()
			try:
				self.do_open( _filename)
			except RuntimeError,e:
				self.reporter.reportError(str(e))

	def on_reloadwarn_toggled(self,*args):
		self.prefs.setBoolPref("Browser","warn_on_reload",self.reloadwarn.get_active())

	def reload_click(self,*args):
		_type = None

		if not self.filename:
			self.reporter.reportError("No file loaded yet!")
			return

		if self.sim:
			_type = self.sim.getType().getName().toString();
		
		if self.reloadwarn.get_active() and self.currentobservertab is not None:
			_alertwin = ReloadDialog(self)
			_reload = _alertwin.run()
			if _reload is False:
				return
		self.library.clear()

		try:
			self.do_open(self.filename)		
			if _type:
				_t = self.library.findType(_type)
				self.do_sim(_t)
		except RuntimeError,e:
			self.reporter.reportError(str(e))

	def props_activate(self,widget,*args):
		return self.modelview.props_activate(self,widget,*args)

	def observe_activate(self,widget,*args):
		return self.modelview.observe_activate(self,widget,*args)

	def solve_click(self,*args):
		#self.reporter.reportError("Solving simulation '" + self.sim.getName().toString() +"'...")
		self.do_solve()

	def console_click(self,*args):
		self.lowertabs.set_current_page(1)
		self.consoletext.grab_focus()

	def integrate_click(self,*args):
		self.do_integrate()
	
	def check_click(self,*args):
		self.do_check()
		#self.reporter.reportError("CHECK clicked")

	def preferences_click(self,*args):
		if not self.sim:
			self.reporter.reportError("No simulation created yet!");		
		self.sim.setSolver(self.solver)
		_params = self.sim.getParameters()
		_paramswin = SolverParametersWindow(
			browser=self
			,params=_params
			,name=self.solver.getName()
		)
		if _paramswin.run() == gtk.RESPONSE_OK:
			print "PARAMS UPDATED"
			self.sim.setParameters(_paramswin.params)
		else:
			print "PARAMS NOT UPDATED"

	def methodrun_click(self,*args):
		_sel = self.methodsel.get_active_text()
		if _sel:
			_method = None
			_methods = self.sim.getType().getMethods()
			for _m in _methods:
				if _m.getName()==_sel:
					_method = _m
			if not _method:
				self.reporter.reportError("Method is not valid")
				return
			self.do_method(_method)
		else:
			self.reporter.reportError("No method selected")
	
	def auto_toggle(self,button,*args):
		self.is_auto = button.get_active()
		if hasattr(self,'automenu'):
			self.automenu.set_active(self.is_auto)
		else:
			raise RuntimeError("no automenu")

		#if self.is_auto:
		#	self.reporter.reportSuccess("Auto mode is now ON")
		#else:
		#	self.reporter.reportSuccess("Auto mode is now OFF")

	def on_file_quit_click(self,*args):
		#self.exit_popup()
		self.do_quit()

	def on_tools_auto_toggle(self,checkmenuitem,*args):
		self.is_auto = checkmenuitem.get_active()
		self.autotoggle.set_active(self.is_auto)

	def on_help_about_click(self,*args):

		self.builder.add_objects_from_file(self.glade_file,["aboutdialog"])
		_about = self.builder.get_object("aboutdialog")
		_about.set_transient_for(self.window);
		_about.set_version(config.VERSION)
		_about.run()
		_about.destroy()

	def on_help_contents_click(self,*args):
		_help = Help(HELP_ROOT)
		_help.run()

	def on_report_a_bug_click(self,*args):
		import urllib
		import platform
		_plat = str(platform.system())
		_version = config.VERSION
		_help = Help(
			url="http://bugs.ascend4.org/bug_report_page.php?project_id=ascend&platform=%s&product_version=%s"
				% (_plat,_version)
		)
		_help.run()

	def on_help_check_for_updates_click(self,*args):
		v = VersionCheck()
		title = "Check for updates"
		text = "Your version is %s\n" % config.VERSION
		self.prefs.setStringPref("Browser","last_update_check","%s" %time.time())
			
		try:
			v.check()
			if config.VERSION==v.latest:
				text += "You are running the latest released version"
			else:
				text += "Latest version is %s\n" % v.latest
				if v.info:
					text += "Get more info at %s\n" % v.info
				if v.download:
					text += "Download from %s\n" % v.download
		except Exception, e:
			text += "\nUnable to check version\n"
			text += str(e)

		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_show_fixable_variables_activate(self,*args):
		try:
			v = self.sim.getFixableVariables()
		except Exception,e:
			self.reporter.reportError("Unable to show fixable variables: %s"%str(e))
			return
		text = "Fixable Variables"
		title = text
		text += "\n"
		if len(v):
			for var in v:
				text += "\n%s"%var
		else:
			text += "\nnone"
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_show_fixed_vars_activate(self,*args):
		try:
			v = self.sim.getFixedVariables()
		except RuntimeError,e:
			self.reporter.reportError("Unable to show fixed variables: %s"%str(e))
			return
		text = "%d fixed variables:" % len(v)
		title = "Fixed Variables"
		text += "\n"
		if len(v):
			for var in v:
				text += "\n%s\t= %f"%(str(var),var.getValue())
		else:
			text += "\nnone"
		_dialog = InfoDialog(self,self.window,text,title,tabs=[100,200])
		_dialog.run()

	def on_show_freeable_variables_activate(self,*args):
		try:
			v = self.sim.getFreeableVariables()
		except RuntimeError,e:
			self.reporter.reportError("Unable to show freeable variables: %s"%str(e))
			return
		text = "Freeable Variables"
		title = text
		text += "\n"
		if len(v):
			for var in v:
				text += "\n%s" % var
		else:
			text += "\nnone"
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()
		
	def on_show_external_functions_activate(self,*args):
		v = self.library.getExtMethods()
		text = "External Functions"
		title = text
		text +="\nHere is the list of external functions currently present in"
		text +=" the Library:"

		if len(v):
			for ext in v:
				text += "\n\n%s (%d inputs, %d outputs):" % \
					(ext.getName(), ext.getNumInputs(), ext.getNumOutputs())
				text += "\n%s" % ext.getHelp()
		else:
			text +="\n\nNone"		
		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_notes_view_activate(self,*args):
		t = None
		try:
			D = self.library.getAnnotationDatabase()
			i = self.modelview.get_selected_instance()
			if i and i.isModel():
				t = i.getType()
			else:
				self.reporter.reportError("First select a MODEL instance");
				return
			v = D.getNotes(t)
		except RuntimeError,e:
			self.reporter.reportError("Unable to show notes: %s"%str(e))
			return
		text = "Notes for '%s'" % t.getName()
		title = text
		text += "\nHere are all notes defined within this MODEL:"
		nn = {}
		if len(v):
			for n in v:
				text += "\n\n%s (%s):" % (n.getId(), n.getLanguage())
				text += "\n\t%s" % n.getText()
		else:
			text += "\n\nThere are no noted defined locally within in this model"

		_dialog = InfoDialog(self,self.window,text,title)
		_dialog.run()

	def on_maintabs_switch_page(self,notebook,page,pagenum):
		#print("Page switched to %d" % pagenum)
		if (pagenum in self.tabs.keys()) and self.tabs[pagenum].alive:
			self.currentobservertab = pagenum
			self.currentpage = pagenum
		else:
			self.currentpage = pagenum
		if pagenum == 1:
			self.enable_on_enter_sim_tab()
			self.modelview.modelview.grab_focus()
		else:
			self.disable_on_leave_sim_tab()

	def create_observer(self,name=None):
		_imagelist = []
		for i in range(5):
			_imagelist.append("image%s" % (i+7))
		self.builder.add_objects_from_file(self.glade_file, _imagelist)
		
		self.builder.add_objects_from_file(self.glade_file,["observervbox","observercontext"])
		
		_label = gtk.Label();
		_tab = self.maintabs.append_page(self.builder.get_object("observervbox"),_label);
		_obs = ObserverTab(name=name, browser=self, tab=_tab)
		_label.set_text(_obs.name)
		self.observers.append(_obs)
		self.tabs[_tab] = _obs
		self.currentobservertab = _tab

		self.builder.get_object("copy_observer_matrix").set_sensitive(True)
		self.builder.get_object("keep_observed").set_sensitive(True)
		return _obs
	
	def sync_observers(self):
		for _o in self.observers:
			_o.sync()
	
	def delete_event(self, widget, event):
		self.do_quit()
		return False
		#return self.exit_popup()

	def exit_popup(self):
		dialog = gtk.MessageDialog(self.window, gtk.DIALOG_MODAL, gtk.MESSAGE_INFO,
				gtk.BUTTONS_YES_NO, "Are you sure you want to Quit?")
		dialog.set_title("Exit popup")
		#dialog.set_default_size(600,300)
		response = dialog.run()
		dialog.destroy()
		if response == gtk.RESPONSE_YES:
			self.do_quit()
			return False
		else:
			return True

	def observe(self,instance):
			if self.currentobservertab is None:
				self.create_observer()
			_observer = self.tabs[self.currentobservertab]
			_observer.add_instance(instance)
			
	def auto_update_check(self):
		_p = self.prefs
		_no_auto_check = _p.getBoolPref("Browser", "disable_auto_check_for_updates", False)
		
		if _no_auto_check is True:
			return
		
		_time_now = time.time()
		_last_update = float(self.prefs.getStringPref("Browser","last_update_check","0"))
		print "Time since last update check : %f days" %((_time_now-_last_update)/(3600*24))
		
		if ((_time_now-_last_update)/(3600*24)) < 7:
			return
		
		_win = AutoUpdateDialog(self)
		_check_now = _win.run()
		
		if _check_now is False:
			return
		
		self.on_help_check_for_updates_click()
	
	def on_check_weekly_toggled(self, widget):
		_p = self.prefs
		_p.setBoolPref("Browser", "disable_auto_check_for_updates", not(widget.get_active()))
		
	def disable_menu(self):
		list=["free_variable","fix_variable","sparsity","propsmenuitem","copy_observer_matrix",
                      "incidencegraph","diagnose_blocks","show_fixed_vars","show_freeable_vars",
                      "show_fixable_variables","show_variables_near_bounds","show_vars_far_from_nominals1",
                      "repaint_tree","checkbutton","solvebutton","integratebutton","methodrunbutton",
                      "check1","solve1","integrate1","units","add_observer","keep_observed","preferences","notes_view"]
		for button in list:
			self.builder.get_object(button).set_sensitive(False)
			
	def disable_on_leave_sim_tab(self):
		list =["free_variable","fix_variable","propsmenuitem","units"]
		for button in list:
			self.builder.get_object(button).set_sensitive(False)
			
	def enable_on_enter_sim_tab(self):
		list =["free_variable","fix_variable","propsmenuitem","units"]
		for button in list:
			self.builder.get_object(button).set_sensitive(False)
		if hasattr(self.modelview,'sim'):
			_path, _col = self.modelview.modelview.get_cursor()
			_instance = None
			if _path:
				_name,_instance = self.modelview.otank[_path]
			if _instance is None:
				return
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
		
	def enable_on_sim_build(self):
		list=["sparsity","incidencegraph","diagnose_blocks","show_fixed_vars","show_freeable_vars",
                      "show_fixable_variables","show_variables_near_bounds","show_vars_far_from_nominals1","notes_view"]
		for button in list:
			self.builder.get_object(button).set_sensitive(True)
	def disable_on_first_run(self):
		list=["reloadbutton","reload","show_external_functions","notes_view"]
		for button in list:
			self.builder.get_object(button).set_sensitive(False)
	def enable_on_file_open(self):
		list=["reloadbutton","reload","show_external_functions"]
		for button in list:
			self.builder.get_object(button).set_sensitive(True)
	def enable_on_model_tree_build(self):
		list=["repaint_tree","checkbutton","solvebutton","integratebutton","methodrunbutton",
                      "check1","solve1","integrate1","units","add_observer","preferences","notes_view"]
		for button in list:
			self.builder.get_object(button).set_sensitive(True)
if __name__ == "__main__":
	b = Browser();
	b.run()
	
class ReloadDialog:
		
	# Just a dialog to confirm that the user REALLY
	# wants to reload the model
	
	def __init__(self, browser):
		browser.builder.add_objects_from_file(browser.glade_file, ["reloaddialog"])
		self.alertwin = browser.builder.get_object("reloaddialog")
		browser.builder.connect_signals(self)
	
	def on_reloaddialog_close(self,*args):
		self.alertwin.response(gtk.RESPONSE_CLOSE)
		
	def run(self):
		_continue = True
		while _continue:
			_res = self.alertwin.run()
			if _res == gtk.RESPONSE_YES:
				self.alertwin.destroy()
				return True
			else:
				self.alertwin.destroy()
				return False
				
class AutoUpdateDialog:
	
	# A dialog to automatically check for updates
	
	def __init__(self, browser):
		self.browser = browser
		browser.builder.add_objects_from_file(browser.glade_file, ["autoupdatedialog"])
		self.win = browser.builder.get_object("autoupdatedialog")
		self.checkbutton = browser.builder.get_object("autoupdate")
		
		_p = self.browser.prefs
		self.checkbutton.set_active(_p.getBoolPref("Browser", "disable_auto_check_for_updates", False))
		
		browser.builder.connect_signals(self)
		
	def on_autoupdate_toggled(self, widget):
		_p = self.browser.prefs
		_p.setBoolPref("Browser", "disable_auto_check_for_updates", widget.get_active())
		self.browser.check_weekly.set_active(not(widget.get_active()))
		
	def run(self):
		_continue = True
		while _continue:
			_res = self.win.run()
			if _res == gtk.RESPONSE_YES:
				self.win.destroy()
				return True
			elif _res == gtk.RESPONSE_NO or _res == gtk.RESPONSE_DELETE_EVENT or _res == gtk.RESPONSE_CLOSE:
				self.win.destroy()
				return False

