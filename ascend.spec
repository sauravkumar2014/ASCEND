Name:		ascend
Summary:	ASCEND modelling environment
Version:	0.9.5.112

# Use release 0.* so that other users can do patch releases with a higher number
# and still have the update occur automatically.
Release:	0%{?dist}

%define disttar_name ascend-0.9.5.112

License:	GPL
Group:		Applications/Engineering
Source:		%{disttar_name}.tar.bz2
URL:		http://ascend.cheme.cmu.edu/

Prefix:		%{_prefix}
Packager:	John Pye
Vendor:		Carnegie Mellon University

Buildroot: /var/tmp/%{name}-buildroot

#----------build dependencies------------

# ...general
BuildRequires: scons >= 0.96.92
BuildRequires: bison >= 2.0
BuildRequires: flex >= 2.5.4
BuildRequires: swig >= 1.3.24

%if %{defined fedora_version}
BuildRequires: gcc-gfortran gcc-c++ >= 4
BuildRequires: blas-devel
BuildRequires: sundials >= 2.2.0
BuildRequires: python-devel >= 2.4
BuildRequires: tk-devel >= 8.3, tcl-devel >= 8.3
BuildRequires: tktable < 2.10, tktable >= 2.8
%else
%if %{defined suse_version}
BuildRequires: gcc-fortran gcc-c++
BuildRequires: sundials >= 2.2.0
BuildRequires: blas
BuildRequires: python-devel >= 2.4
BuildRequires: tk-devel >= 8.3, tcl-devel >= 8.3
BuildRequires: tktable < 2.10, tktable >= 2.8
%else
%if %{defined mandriva_version}
BuildRequires: gcc-gfortran gcc-c++
BuildRequires: sundials >= 2.2.0
BuildRequires: blas-devel python-devel tk tcl
%else
# xubuntu version is the fallback...
BuildRequires: g++-4.1 gfortran-4.1 libsundials-serial-dev python-dev tk8.3-dev tcl8.3-dev tktable
%endif
%endif
%endif

#BuildRequires: conopt >= 3.14

# ... GNOME
BuildRequires: desktop-file-utils

%define pyver %(python -c 'import sys ; print sys.version[:3]')
%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Buildroot: /var/tmp/%{name}-buildroot

%description
ASCEND IV is both a large-scale object-oriented mathematical
modeling environment and a strongly typed mathematical modeling
language. Although ASCEND has primarily been developed by Chemical
Engineers, great care has been exercised to assure that it is
domain independent. ASCEND can support modeling activities in
fields from Architecture to (computational) Zoology.

# ... documentation
# There are no dependencies for documentation as the tarball
# will always contain documentation in compiled form. Only
# when building from subversion are targets formats of the
# documentation files not available.

#-----------runtime dependencies-----------

# ...general
Requires: gtksourceview
Requires: blas
# sundials will be statically linked
Requires: desktop-file-utils shared-mime-info

# ...pygtk
Requires: python >= 2.4
Requires: pygtk2 >= 2.6
Requires: pygtk2-libglade
Requires: python-matplotlib
Requires: numpy
Requires: ipython

# ...tcl/tk
Requires: xgraph >= 11
Requires: tcl >= 8.3
Requires: tk >= 8.3
Requires: tktable < 2.10, tktable >= 2.8

#------------------------------------------

Provides: ascend-gui

# for the moment we'll just make one big super-package, to keep things 
# simple for end-users.

#%package -n ascend-python
#Version:    %{version}
#Summary:    PyGTK user interface for ASCEND
#Group:		Applications/Engineering
#
#%description -n ascend-python
#PyGTK user interface for ASCEND. This is a new interface that follows GNOME
#human interface guidelines as closely as possible. It does not as yet provide
#access to all of the ASCEND functionality provided by the Tcl/Tk interface.
#
#%package -n ascend-tcltk
#Version:    %{version}
#Summary:    Tcl/Tk user interface for ASCEND
#Group:		Applications/Engineering
#
#%description -n ascend-tcltk
#Tcl/Tk user interface for ASCEND. This is the original ASCEND IV interface
#and is a more complete and mature interface than the alternative PyGTK
#interface. Use this interface if you need to use ASCEND *.a4s files or other
#functionality not provided by the PyGTK interface.

%package doc
Summary: Documentation for ASCEND
Group: Applications/Engineering
%description doc
Documentation for ASCEND, in the form of a PDF User's Manual.

%package devel
Summary: Developer files ASCEND
Group: Applications/Engineering
Requires: %{name}
%description devel
Developer files for ASCEND, in the form for C header files for the core
ASCEND library, 'libascend'.

%prep
%setup -q -n %{disttar_name}

%build
scons %{_smp_mflags} DEFAULT_ASCENDLIBRARY=%{_datadir}/ascend/models \
	INSTALL_ROOT=%{buildroot} \
	INSTALL_PREFIX=%{_prefix} \
	INSTALL_SHARE=%{_datadir} \
	INSTALL_BIN=%{_bindir} \
	INSTALL_INCLUDE=%{_includedir} \
	INSTALL_LIB=%{_libdir} \
	INSTALL_DOC=%{_docdir}/%{name}-%{version} \
	WITH_DOC_BUILD=0 \
	WITH_DOC_INSTALL=0 \
	WITH_SOLVERS=QRSLV,LSODE,CMSLV,IDA,LRSLV,CONOPT \
	ABSOLUTE_PATHS=1 \
	%{?__cc:CC="%__cc"} %{?__cxx:CXX="%__cxx"} \
	pygtk tcltk models

%install
rm -rf %{buildroot}
scons %{_smp_mflags} install

# Install menu entry for PyGTK interface, gtksourceview syntax highlighting, and MIME definition
pushd pygtk/gnome
install -m 644 -D ascend.desktop %{buildroot}/%{_datadir}/applications/ascend.desktop
install -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/ascend-app.png
install -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/ascend.png
install -m 644 -D ascend.xml %{buildroot}/%{_datadir}/mime/packages/ascend.xml
popd

# file-type icon for ascend models (double click should open in ASCEND)
pushd pygtk/glade
install -m 644 -D ascend.svg %{buildroot}/%{_datadir}/icons/text-x-ascend-model.svg
popd

pushd tools/gedit
install -m 644 -D ascend.lang %{buildroot}/%{_datadir}/gtksourceview-1.0/language-specs/ascend.lang
popd

# TODO...
#%__python -c 'from compileall import *; compile_dir("'$RPM_BUILD_ROOT'/%{python_sitelib}",10,"%{python_sitelib}")'
#%__python -O -c 'from compileall import *; compile_dir("'$RPM_BUILD_ROOT'/%{python_sitelib}",10,"%{python_sitelib}")'

# Install menu entry for Tcl/Tk interface
pushd tcltk/gnome
install -m 644 -D ascend4.desktop %{buildroot}/%{_datadir}/applications/ascend4.desktop
install -m 644 -D ascend4.png %{buildroot}/%{_datadir}/icons/ascend4-app.png
install -m 644 -D ascend4.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/ascend4.png
popd

%clean
rm -rf %{buildroot}

%post
update-desktop-database
update-mime-database /usr/share/mime &> /dev/null || :

%postun
update-desktop-database
update-mime-database /usr/share/mime &> /dev/null || :

%files
%defattr(-, root, root)
%doc INSTALL.txt LICENSE.txt

%{_bindir}/ascend-config
%{_datadir}/ascend/models
%{_datadir}/ascend/solvers
%{_libdir}/libascend.so
%{_datadir}/mime/packages/ascend.xml
%{_datadir}/gtksourceview-1.0/language-specs/ascend.lang
%{_datadir}/icons/text-x-ascend-model.svg

# %package -n ascend-python
%{_bindir}/ascend
%{_datadir}/ascend/*.py
%{_datadir}/ascend/*.py[co]

%{_datadir}/ascend/glade
%{_datadir}/ascend/_ascpy.so
%{_datadir}/applications/ascend.desktop
%{_datadir}/icons/ascend-app.png
%{_datadir}/icons/hicolor/64x64/ascend.png

# %package -n ascend-tcltk
%{_bindir}/ascend4
%{_datadir}/ascend/tcltk
%{_libdir}/libascendtcl.so
%{_datadir}/applications/ascend4.desktop
%{_datadir}/icons/ascend4-app.png
%{_datadir}/icons/hicolor/64x64/ascend4.png

%files devel
%{_includedir}/compiler
%{_includedir}/general
%{_includedir}/utilities
%{_includedir}/solver
%{_includedir}/linear
%{_includedir}/integrator
%{_includedir}/system

%files doc
%doc doc/book.pdf

%changelog
* Mon Apr 23 2007 John Pye <john.pye@anu.edu.au> 0.9.5.112
- solvers are now all built as separate shared libraries

* Mon Apr 23 2007 John Pye <john.pye@student.unsw.edu.au> 0.9.5.108
- File ascend.lang has moved.
- book.pdf is included in package.
- some header files have been moved.

* Fri Jul 28 2006 John Pye <john.pye@student.unsw.edu.au>
- Added CONOPT support

* Wed Jul 12 2006 John Pye <john.pye@student.unsw.edu.au>
- Fixed fortran linking
- Removed ccache dependency
- Added xgraph dependency
- Added 'include' files plus 'ascend-config' script

* Thu Jun 01 2006 John Pye <john.pye@student.unsw.edu.au>
- Add Tcl/Tk interface to GNOME menu

* Tue May 02 2006 John Pye <john.pye@student.unsw.edu.au>
- Break out ascend-core, ascend-python and ascend-tcltk packages.

* Mon Apr 24 2006 John Pye <john.pye@student.unsw.edu.au>
- Modified for removed dir in pygtk source hierachy

* Thu Apr 04 2006 John Pye <john.pye@student.unsw.edu.au>
- First RPM package for new SCons build
