import os, commands, platform, distutils.sysconfig, os.path

version = "0.9.6rc0"

#------------------------------------------------------
# OPTIONS
#
# Note that if you set the options via the command line, they will be
# remembered in the file 'options.cache'. It's a feature ;-)

opts = Options(['options.cache', 'config.py'])
print "PLATFORM = ",platform.system()

# Import the outside environment
env = Environment(ENV=os.environ)

if platform.system()=='Windows' and env.has_key('MSVS'):
	print "INCLUDE =",env['ENV']['INCLUDE']
	print "LIB =",env['ENV']['LIB']
	print "LINK =",env['LINK']
	print "LINKCOM =",env['LINKCOM']
	print "AR =",env['AR']
	print "ARCOM =",env['ARCOM']
	#env['AR']='link /lib'
	env.Append(CPPPATH=env['ENV']['INCLUDE'])
	env.Append(LIBPATH=env['ENV']['LIB'])
	env.Append(CPPDEFINES=['_CRT_SECURE_NO_DEPRECATED','_CRT_SECURE_NO_DEPRECATE'])

# Package linking option
opts.Add(EnumOption(
	'PACKAGE_LINKING'
	, 'Style of linking for external libraries'
	, 'DYNAMIC_PACKAGES'
    , ['DYNAMIC_PACKAGES', 'STATIC_PACKAGES', 'NO_PACKAGES']
))

# You can turn off building of Tcl/Tk interface
opts.Add(BoolOption(
	'WITHOUT_TCLTK_GUI'
	,"Set to True if you don't want to build the original Tcl/Tk GUI."
	, False
))

# You can turn off the building of the Python interface
opts.Add(BoolOption(
	'WITHOUT_PYTHON'
	,"Set to True if you don't want to build Python wrappers."
	, False
))

# Which solvers will we allow?
opts.Add(ListOption(
	'WITH_SOLVERS'
	,"List of the solvers you want to build. The default is the minimum that"	
		+" works."
	,["QRSLV","CMSLV"]
	,['QRSLV','MPS','SLV','OPTSQP'
		,'NGSLV','CMSLV','LRSLV','MINOS','CONOPT'
		,'LSOD','OPTSQP'
	 ]
))

# Where will the local copy of the help files be kept?
opts.Add(PackageOption(
	'WITH_LOCAL_HELP'
	, "Directory containing the local copy of the help files (optional)"
	, "no"
))

# Will bintoken support be enabled?
opts.Add(BoolOption(
	'WITH_BINTOKEN'
	,"Enable bintoken support? This means compiling models as C-code before"
		+" running them, to increase solving speed for large models."
	,False
))

# What should the default ASCENDLIBRARY path be?
# Note: users can change it by editing their ~/.ascend.ini
opts.Add(
	'DEFAULT_ASCENDLIBRARY'
	,"Set the default value of the ASCENDLIBRARY -- the location where"
		+" ASCEND will look for models when running ASCEND"
	,os.path.expanduser("~/src/ascend/trunk/models")
)

# Where is SWIG?
opts.Add(
	'SWIG'
	,"SWIG location, probably only required for MinGW and MSVC users."
		+" Enter the location as a Windows-style path, for example"
		+" 'c:\\msys\\1.0\\home\\john\\swigwin-1.3.29\\swig.exe'."
)

# Build the test suite?
opts.Add(BoolOption(
	'WITH_CUNIT_TESTS'
	,"Whether to build the CUnit tests. Default is off. If set to on,"
		+" you must have CUnit installed somewhere that SCons can"
		+" find it."
	,False
))

# Where are the CUnit includes?
opts.Add(PackageOption(
	'CUNIT_CPPPATH'
	,"Where are your CUnit include files?"
	,None
))

# Where are the CUnit libraries?
opts.Add(PackageOption(
	'CUNIT_LIBPATH'
	,"Where are your CUnit libraries?"
	,None
))

# Where are the Tcl includes?
opts.Add(PackageOption(
	'TCL_CPPPATH'
	,"Where are your Tcl include files?"
	,None
))

# Where are the Tcl libs?
opts.Add(PackageOption(
	'TCL_LIBPATH'
	,"Where are your Tcl libraries?"
	,None
))

# What is the name of the Tcl lib?
opts.Add(
	'TCL_LIB'
	,"Name of Tcl lib (eg 'tcl' or 'tcl83')"
	,'tcl'
)

# Where are the Tk includes?
opts.Add(PackageOption(
	'TK_CPPPATH'
	,"Where are your Tk include files?"
	,None
))

# Where are the Tk libs?
opts.Add(PackageOption(
	'TK_LIBPATH'
	,"Where are your Tk libraries?"
	,None
))

# What is the name of the Tk lib?
opts.Add(
	'TK_LIB'
	,"Name of Tk lib (eg 'tk' or 'tk83')"
	,'tk'
)	

opts.Add(
	'INSTALL_PREFIX'
	,'Root location for installed files'
	,'/usr/local'
)

opts.Add(
	'INSTALL_BIN'
	,'Location to put binaries during installation'
	,"$INSTALL_PREFIX/bin"
)

opts.Add(
	'INSTALL_DATA'
	,'Location to put data files during installation'
	,"$INSTALL_PREFIX/share"
)

opts.Add(
	'INSTALL_INCLUDE'
	,'Location to put header files during installation'
	,"$INSTALL_PREFIX/include"
)

if platform.system()=="Windows":
	default_install_assets = "glade/"
	icon_extension = '.png'
else:
	default_install_assets = "$INSTALL_DATA/ascend/glade/"
	icon_extension = '.svg'

opts.Add(
	'PYGTK_ASSETS'
	,'Default location for Glade assets (placed in pygtk/interface/config.py)'
	,default_install_assets
)

opts.Add(
	'INSTALL_ROOT'
	,'For use by RPM only: location of %{buildroot} during rpmbuild'
	,""
)

# TODO: OTHER OPTIONS?
# TODO: flags for optimisation
# TODO: turning on/off bintoken functionality
# TODO: Where will the 'Makefile.bt' file be installed?

opts.Update(env)
opts.Save('options.cache',env)

Help(opts.GenerateHelpText(env))

with_tcltk_gui = (env['WITHOUT_TCLTK_GUI']==False)
without_tcltk_reason = "disabled by options/config.py"

with_python = (env['WITHOUT_PYTHON']==False)
without_python_reason = "disabled by options/config.py"

with_cunit_tests = env['WITH_CUNIT_TESTS']
without_cunit_reason = "not requested"

print "SOLVERS:",env['WITH_SOLVERS']
print "WITH_LOCAL_HELP:",env['WITH_LOCAL_HELP']
print "WITH_BINTOKEN:",env['WITH_BINTOKEN']
print "DEFAULT_ASCENDLIBRARY:",env['DEFAULT_ASCENDLIBRARY']

subst_dict = {
	'@WEBHELPROOT@':'http://pye.dyndns.org/ascend/manual/'
	, '@GLADE_FILE@':'ascend.glade'
	, '@DEFAULT_ASCENDLIBRARY@':env['DEFAULT_ASCENDLIBRARY']
	, '@ICON_EXTENSION@':icon_extension
	, '@HELP_ROOT@':''
	, '@INSTALL_DATA@':env['INSTALL_DATA']
	, '@INSTALL_BIN@':env['INSTALL_BIN']
	, '@INSTALL_INCLUDE@':env['INSTALL_INCLUDE']
	, '@PYGTK_ASSETS@':env['PYGTK_ASSETS']
	, '@VERSION@':version
}

if env['WITH_LOCAL_HELP']:
	subst_dict['@HELP_ROOT@']=env['WITH_LOCAL_HELP']
		
env.Append(SUBST_DICT=subst_dict)

#------------------------------------------------------
# SPECIAL CONFIGURATION TESTS

#----------------
# SWIG

import os,re

need_fortran = False

def get_swig_version(env):
	cmd = env['SWIG']+' -version'
	(cin,coutcerr) = os.popen4(cmd)
	output = coutcerr.read()
	
	restr = "SWIG\\s+Version\\s+(?P<maj>[0-9]+)\\.(?P<min>[0-9]+)\\.(?P<pat>[0-9]+)\\s*$"
	expr = re.compile(restr,re.M);
	m = expr.search(output);
	if not m:
		return None
	maj = int(m.group('maj'))
	min = int(m.group('min'))
	pat = int(m.group('pat'))

	return (maj,min,pat)
	

def CheckSwigVersion(context):
	
	try:
		context.Message("Checking version of SWIG... ")
		maj,min,pat = get_swig_version(context.env)
	except:
		context.Result("Failed to detect version, or failed to run SWIG")
		return 0;
	
	if maj == 1 and (
			min > 3
			or (min == 3 and pat >= 24)
		):
		context.Result("ok, %d.%d.%d" % (maj,min,pat))
		return 1;
	else:
		context.Result("too old, %d.%d.%d" % (maj,min,pat))
		return 0;

#----------------
# General purpose library-and-header test

class KeepContext:
	def __init__(self,context,varprefix):
		self.keep = {}
		for k in ['LIBS','LIBPATH','CPPPATH']:
			if context.env.has_key(k):
				self.keep[k] = context.env[k]
		
		libpath_add = []
		if context.env.has_key(varprefix+'_LIBPATH'):
			libpath_add = [env[varprefix+'_LIBPATH']]
			#print "Adding '"+str(libpath_add)+"' to lib path"

		cpppath_add = []
		if context.env.has_key(varprefix+'_CPPPATH'):
			cpppath_add = [env[varprefix+'_CPPPATH']]
			#print "Adding '"+str(cpppath_add)+"' to cpp path"

		libs_add = []
		if context.env.has_key(varprefix+'_LIB'):
			libs_add = [env[varprefix+'_LIB']]
			#print "Adding '"+str(libs_add)+"' to libs"	

		context.env.Append(
			LIBPATH = libpath_add
			, CPPPATH = cpppath_add
			, LIBS = libs_add
		)

	def restore(self,context):
		for k in self.keep:
			context.env[k]=self.keep[k];

def CheckExtLib(context,libname,text,ext='.c',varprefix=None):
	"""This method will check for variables LIBNAME_LIBPATH
	and LIBNAME_CPPPATH and try to compile and link the 
	file with the provided text, linking with the 
	library libname."""

	context.Message( 'Checking for '+libname+'... ' )
	
	if varprefix==None:
		varprefix = libname.upper()
	
	keep = KeepContext(context,varprefix)

#	print "TryLink with CPPPATH="+str(context.env['CPPPATH'])
#	print "TryLink with LIBS="+str(context.env['LIBS'])
#	print "TryLink with LIBPATH="+str(context.env['LIBPATH'])

	if not context.env.has_key(varprefix+'_LIB'):
		context.env.Append(LIBS=libname)

	is_ok = context.TryLink(text,ext)
	
#	print "Link success? ",(is_ok != 0)

	keep.restore(context)

#	print "Restored CPPPATH="+str(context.env['CPPPATH'])
#	print "Restored LIBS="+libname
#	print "Restored LIBPATH="+str(context.env['LIBPATH'])

	context.Result(is_ok)
	return is_ok

#----------------
# CUnit test

cunit_test_text = """
#include <CUnit/CUnit.h>
int maxi(int i1, int i2){
	return (i1 > i2) ? i1 : i2;
}

void test_maxi(void){
	CU_ASSERT(maxi(0,2) == 2);
	CU_ASSERT(maxi(0,-2) == 0);
	CU_ASSERT(maxi(2,2) == 2);

}
int main(void){
/* 	CU_initialize_registry() */
	return 0;
}
"""

def CheckCUnit(context):
	return CheckExtLib(context,'cunit',cunit_test_text)

#----------------
# Tcl test

tcl_check_text = r"""
#include <tcl.h>
#include <stdio.h>
int main(void){
    printf("%s",TCL_PATCH_LEVEL);
	return 0;
}
"""

def CheckTcl(context):
	return CheckExtLib(context,'tcl',tcl_check_text)

def CheckTclVersion(context):
	keep = KeepContext(context,'TCL')
	context.Message("Checking Tcl version... ")
	(is_ok,output) = context.TryRun(tcl_check_text,'.c')
	keep.restore(context)
	if not is_ok:
		context.Result("failed to run check")
		return 0

	major,minor,patch = tuple(int(i) for i in output.split("."))
	if major != 8 or minor > 3:
		context.Result(output+" (bad version)")
		# bad version
		return 0
		
	# good version
	context.Result(output+" (good)")
	return 1

#----------------
# Tcl test

tk_check_text = r"""
#include <tk.h>
#include <stdio.h>
int main(void){
    printf("%s",TK_PATCH_LEVEL);
	return 0;
}
"""
def CheckTk(context):
	return CheckExtLib(context,'tk',tcl_check_text)


def CheckTkVersion(context):
	keep = KeepContext(context,'TK')
	context.Message("Checking Tk version... ")
	(is_ok,output) = context.TryRun(tk_check_text,'.c')
	keep.restore(context)
	if not is_ok:
		context.Result("failed to run check")
		return 0
	context.Result(output)

	major,minor,patch = tuple(int(i) for i in output.split("."))
	if major != 8 or minor > 3:
		# bad version
		return 0
		
	# good version
	return 1
	
#------------------------------------------------------
# CONFIGURATION

conf = Configure(env
	, custom_tests = { 
		'CheckSwigVersion' : CheckSwigVersion
		, 'CheckCUnit' : CheckCUnit
		, 'CheckTcl' : CheckTcl
		, 'CheckTclVersion' : CheckTclVersion
		, 'CheckTk' : CheckTk
		, 'CheckTkVersion' : CheckTkVersion
#		, 'CheckIsNan' : CheckIsNan
#		, 'CheckCppUnitConfig' : CheckCppUnitConfig
	} 
	, config_h = "config.h"
)


# Math library

#if not conf.CheckFunc('sinh') and not conf.CheckLibWithHeader(['m','c','libc'], 'math.h', 'C'):
#	print 'Did not find math library, exiting!'
#	Exit(1)

# Where is 'isnan'?

if not conf.CheckFunc('isnan'):
	print "Didn't find isnan"
#	Exit(1)

# Tcl/Tk

if conf.CheckTcl():
	if with_tcltk_gui and conf.CheckTclVersion():
		if conf.CheckTk():
			if with_tcltk_gui and not conf.CheckTkVersion():
				without_tcltk_reason = "Require Tk version <= 8.3. See 'scons -h'"
				with_tcltk_gui = False
		else:
			without_tcltk_reason = "Tk not found."
			with_tcltk_gui = False
	else:
		without_tcltk_reason = "Require Tcl <= 8.3 Tcl."
		with_tcltk_gui = False

else:
	without_tcltk_reason = "Tcl not found."
	with_tcltk_gui = False

# Python... obviously we're already running python, so we just need to
# check that we can link to the python library OK:

if platform.system()=="Windows":
	python_lib='python24'
else:
	python_lib='python2.4'

# SWIG version

if platform.system()=="Windows":
	env['ENV']['SWIGFEATURES']='-O'
else:
	env['ENV']['SWIGFEATURES']='-O'	


if not conf.CheckSwigVersion():
	without_python_reason = 'SWIG >= 1.3.24 is required'
	with_python = False

# CUnit

if with_cunit_tests:
	if not conf.CheckCUnit():
		without_cunit_reason = 'CUnit not found'

# BLAS

if conf.CheckLib('blas'):
	print "FOUND BLAS"
	with_local_blas = False
	without_local_blas_reason = "Found BLAS installed on system"
else:
	print "DIDN'T FIND BLAS"
	with_local_blas = True
	need_fortran = True

# FORTRAN

if need_fortran:
	conf.env.Tool('f77')
	detect_fortran = conf.env.Detect(['g77','f77'])
	if detect_fortran:
		# For some reason, g77 doesn't get detected properly on MinGW
		if not env.has_key('F77'):
			conf.env.Replace(F77=detect_fortran)
			conf.env.Replace(F77COM='$F77 $F77FLAGS -c -o $TARGET $SOURCE')
			conf.env.Replace(F77FLAGS='')
			#print "F77:",conf.env['F77']
			#print "F77COM:",conf.env['F77COM']
			#print "F77FLAGS:",conf.env['F77FLAGS']
			fortran_builder = Builder(
				action='$F77COM'
				, suffix='.o'
				, src_suffix='.f'
			)
			conf.env.Append(BUILDERS={'Fortran':fortran_builder})
	else:
		print "FORTRAN-77 required but not found"
		Exit(1)
else:
	print "FORTRAN not required"

# TODO: -D_HPUX_SOURCE is needed

# TODO: check size of void*

# TODO: detect if dynamic libraries are possible or not

if platform.system()=="Windows" and env.has_key('MSVS'):
	if not conf.CheckHeader('windows.h') and env['PACKAGE_LINKING']=='DYNAMIC_PACKAGES':
		print "Reverting to STATIC_PACKAGES since windows.h is not available. Probably you "\
			+"need to install the Microsoft Windows Server 2003 Platform SDK, or similar."
		env['PACKAGE_LINKING']='STATIC_PACKAGES'
		
	if with_python and not conf.CheckHeader('basetsd.h'):
		with_python = 0;
		without_python_reason = "Header file 'basetsd.h' not found. Install the MS Platform SDK."

conf.env.Append(CPPDEFINES=env['PACKAGE_LINKING'])

conf.Finish()

env.Append(PYTHON_LIBPATH=[distutils.sysconfig.PREFIX+"/libs"])
env.Append(PYTHON_LIB=[python_lib])
env.Append(PYTHON_CPPPATH=[distutils.sysconfig.get_python_inc()])

#------------------------------------------------------
# RECIPE: 'SubstInFile', used in pygtk SConscript

import re
from SCons.Script import *  # the usual scons stuff you get in a SConscript

def TOOL_SUBST(env):
    """Adds SubstInFile builder, which substitutes the keys->values of SUBST_DICT
    from the source to the target.
    The values of SUBST_DICT first have any construction variables expanded
    (its keys are not expanded).
    If a value of SUBST_DICT is a python callable function, it is called and
    the result is expanded as the value.
    If there's more than one source and more than one target, each target gets
    substituted from the corresponding source.
    """
    env.Append(TOOLS = 'SUBST')
    def do_subst_in_file(targetfile, sourcefile, dict):
        """Replace all instances of the keys of dict with their values.
        For example, if dict is {'%VERSION%': '1.2345', '%BASE%': 'MyProg'},
        then all instances of %VERSION% in the file will be replaced with 1.2345 etc.
        """
        try:
            f = open(sourcefile, 'rb')
            contents = f.read()
            f.close()
        except:
            raise SCons.Errors.UserError, "Can't read source file %s"%sourcefile
        for (k,v) in dict.items():
            contents = re.sub(k, v, contents)
        try:
            f = open(targetfile, 'wb')
            f.write(contents)
            f.close()
        except:
            raise SCons.Errors.UserError, "Can't write target file %s"%targetfile
        return 0 # success

    def subst_in_file(target, source, env):
        if not env.has_key('SUBST_DICT'):
            raise SCons.Errors.UserError, "SubstInFile requires SUBST_DICT to be set."
        d = dict(env['SUBST_DICT']) # copy it
        for (k,v) in d.items():
            if callable(v):
                d[k] = env.subst(v())
            elif SCons.Util.is_String(v):
                d[k]=env.subst(v)
            else:
                raise SCons.Errors.UserError, "SubstInFile: key %s: %s must be a string or callable"%(k, repr(v))
        for (t,s) in zip(target, source):
            return do_subst_in_file(str(t), str(s), d)

    def subst_in_file_string(target, source, env):
        """This is what gets printed on the console."""
        return '\n'.join(['Substituting vars from %s into %s'%(str(s), str(t))
                          for (t,s) in zip(target, source)])

    def subst_emitter(target, source, env):
        """Add dependency from substituted SUBST_DICT to target.
        Returns original target, source tuple unchanged.
        """
        d = env['SUBST_DICT'].copy() # copy it
        for (k,v) in d.items():
            if callable(v):
                d[k] = env.subst(v())
            elif SCons.Util.is_String(v):
                d[k]=env.subst(v)
        Depends(target, SCons.Node.Python.Value(d))
        return target, source

    subst_action=SCons.Action.Action(subst_in_file, subst_in_file_string)
    env['BUILDERS']['SubstInFile'] = Builder(action=subst_action, emitter=subst_emitter)

TOOL_SUBST(env)

#------------------------------------------------------
 # Recipe for 'CHMOD' ACTION 	 
  	 
import SCons 	 
from SCons.Script.SConscript import SConsEnvironment 	 
SConsEnvironment.Chmod = SCons.Action.ActionFactory(os.chmod, 	 
	lambda dest, mode: 'Chmod("%s", 0%o)' % (dest, mode)) 	 
  	 
def InstallPerm(env, dest, files, perm): 	 
	obj = env.Install(dest, files) 	 
	for i in obj: 	 
		env.AddPostAction(i, env.Chmod(str(i), perm)) 	 
  	 
SConsEnvironment.InstallPerm = InstallPerm 	 
  	 
# define wrappers 	 
SConsEnvironment.InstallProgram = lambda env, dest, files: InstallPerm(env, dest, files, 0755) 	 
SConsEnvironment.InstallHeader = lambda env, dest, files: InstallPerm(env, dest, files, 0644) 	 
  	 
#------------------------------------------------------
# SUBDIRECTORIES....


env.Append(CPPPATH=['..'])

env.SConscript(['base/generic/general/SConscript'],'env')

env.SConscript(['base/generic/utilities/SConscript'],'env')

env.SConscript(['base/generic/compiler/SConscript'],'env')

env.SConscript(['base/generic/solver/SConscript'],'env')

env.SConscript(['base/generic/packages/SConscript'],'env')

if with_tcltk_gui:
	env.SConscript(['tcltk98/generic/interface/SConscript'],'env')
else:
	print "Skipping... Tcl/Tk GUI isn't being built:",without_tcltk_reason

if with_python:
	env.SConscript(['pygtk/interface/SConscript'],'env')
else:
	print "Skipping... Python GUI isn't being built:",without_python_reason

if with_cunit_tests:
	testdirs = ['general','solver','utilities']
	for testdir in testdirs:
		path = 'base/generic/'+testdir+'/test/'
		env.SConscript([path+'SConscript'],'env')
	env.SConscript(['test/SConscript'],'env')
	env.SConscript(['base/generic/test/SConscript'],'env')
	
	
else:
	print "Skipping... CUnit tests aren't being built:",without_cunit_reason

if with_tcltk_gui:
	if with_local_blas:
		env.SConscript(['blas/SConscript'],'env')
	else:
		print "Skipping... BLAS won't be build:", without_local_blas_reason

	env.SConscript(['lsod/SConscript'],'env')		

	env.SConscript(['linpack/SConscript'],'env')

# the models directory only needs to be processed for installation
env.SConscript(['models/SConscript'],'env')

#------------------------------------------------------
# INSTALLATION

install_dirs = [env['INSTALL_ROOT']+env['INSTALL_BIN']]+[env['INSTALL_ROOT']+env['INSTALL_DATA']]

# TODO: add install options
env.Alias('install',install_dirs)

#------------------------------------------------------
# CREATE the SPEC file for generation of RPM packages

env.SubstInFile('ascend.spec.in')
