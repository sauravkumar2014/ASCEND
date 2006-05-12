Name:		ascend
Summary:	ASCEND modelling environment
Version:	0.9.5.91

# Use release "0" so that distro-released versions will override ours.
Release:	0.jdpipe

License:	GPL
Group:		Applications/Engineering
Source:		ascend.tar.bz2
URL:		http://ascend.cheme.cmu.edu/

Prefix:		%{_prefix}
Packager:	John Pye
Vendor:		Carnegie Mellon University
BuildRoot:	%{_tmppath}/%{name}-%{version}-root

BuildRequires: python >= 2.4, python-devel
BuildRequires: scons >= 0.96.1
BuildRequires: bison, flex
BuildRequires: swig >= 1.3.24
BuildRequires: desktop-file-utils
BuildRequires: tk-devel < 8.5
BuildRequires: tcl-devel < 8.5
BuildRequires: tktable < 2.10, tktable >= 2.8
BuildRequires: ccache

Requires: python >= 2.4
Requires: pygtk2 >= 2.6
Requires: pygtk2-libglade
Requires: python-matplotlib
Requires: python-numeric
Requires: gtksourceview

%description
ASCEND IV is both a large-scale object-oriented mathematical
modeling environment and a strongly typed mathematical modeling
language. Although ASCEND has primarily been developed by Chemical
Engineers, great care has been exercised to assure that it is
domain independent. ASCEND can support modeling activities in
fields from Architecture to (computational) Zoology.

#%package -n ascend-python
#Version:    0.9.5.91
#Summary:    PyGTK user interface for ASCEND
#Group:		Applications/Engineering
#
#%description -n ascend-python
#PyGTK user interface for ASCEND. This is a new interface that follows GNOME
#human interface guidelines as closely as possible. It does not as yet provide
#access to all of the ASCEND functionality provided by the Tcl/Tk interface.
#
#%package -n ascend-tcltk
#Version:    0.9.5.91
#Summary:    Tcl/Tk user interface for ASCEND
#Group:		Applications/Engineering
#
#%description -n ascend-tcltk
#Tcl/Tk user interface for ASCEND. This is the original ASCEND IV interface
#and is a more complete and mature interface than the alternative PyGTK
#interface. Use this interface if you need to use ASCEND *.a4s files or other
#functionality not provided by the PyGTK interface.

%prep
%setup -q -n ascend

%build
scons %{?_smp_mflags} CC="ccache gcc" CXX="ccache g++" DEFAULT_ASCENDLIBRARY=%{_datadir}/ascend/models INSTALL_ROOT=%{buildroot} INSTALL_PREFIX=%{_prefix} INSTALL_DATA=%{_datadir} INSTALL_BIN=%{_bindir} INSTALL_INCLUDE=%{_incdir} WITH_PYTHON=1 WITH_TCLTK=1 TCL=/usr TCL_LIB=tcl8.4 TK_LIB=tk8.4

%install
rm -rf %{buildroot}
scons %{?_smp_mflags} CC="ccache gcc" CXX="ccache g++" DEFAULT_ASCENDLIBRARY=%{_datadir}/ascend/models INSTALL_ROOT=%{buildroot} INSTALL_PREFIX=%{_prefix} INSTALL_DATA=%{_datadir} INSTALL_BIN=%{_bindir} INSTALL_INCLUDE=%{_incdir} WITH_PYTHON=1 WITH_TCLTK=1 install

pushd pygtk/gnome
install -m 644 -D ascend.desktop %{buildroot}/%{_datadir}/applications/ascend.desktop
install -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/ascend-app.png
install -m 644 -D ascend.png %{buildroot}/%{_datadir}/icons/hicolor/64x64/ascend.png
install -m 644 -D ascend.xml %{buildroot}/%{_datadir}/mime/packages/ascend.xml
install -m 644 -D ascend.lang %{buildroot}/%{_datadir}/gtksourceview-1.0/language-specs/ascend.lang
popd

%clean
rm -rf %{buildroot}

%post
update-desktop-database
update-mime-database /usr/share/mime

%postun
update-desktop-database
update-mime-database /usr/share/mime

%files
%defattr(-, root, root)
%doc INSTALL.txt LICENSE.txt
%{_datadir}/applications/ascend.desktop
%{_datadir}/ascend/models
%{_libdir}/libascend.so

# %package -n ascend-python
%{_bindir}/ascend
%{_datadir}/gtksourceview-1.0/language-specs/ascend.lang
%{_datadir}/icons/ascend-app.png
%{_datadir}/icons/hicolor/64x64/ascend.png
%{_datadir}/mime/packages/ascend.xml
%{_datadir}/ascend/*.py
%{_datadir}/ascend/*.pyc
%{_datadir}/ascend/*.pyo
%{_datadir}/ascend/glade
%{_datadir}/ascend/_ascpy.so

# %package -n ascend-tcltk
%{_bindir}/ascend4
%{_datadir}/ascend/tcltk
%{_libdir}/libascendtcl.so

%changelog
* Tue May 02 2006 John Pye <john.pye@student.unsw.edu.au>
- Break out ascend-core, ascend-python and ascend-tcltk packages.

* Mon Apr 24 2006 John Pye <john.pye@student.unsw.edu.au>
- Modified for removed dir in pygtk source hierachy

* Thu Apr 04 2006 John Pye <john.pye@student.unsw.edu.au>
- First RPM package for new SCons build
