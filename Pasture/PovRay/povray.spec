Name:          povray
Version:       3.7.0.0
Release:       1%{?dist}
License:       AGPL3
Summary:       Pov-Ray Persistence of Vision Raytracer
URL:           http://www.povray.org
Source:        povray-3.7.0.0.tar.gz
Group:         Applications/Multimedia
BuildRoot:     %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: boost-devel
BuildRequires: libjpeg-devel
BuildRequires: libpng-devel
BuildRequires: libtiff-devel 
BuildRequires: zlib-devel 
BuildRequires: OpenEXR-devel

%description
The Persistence of Vision Raytracer is a high-quality, totally
free tool for creating stunning three-dimensional graphics. It
is available in official versions for Windows, Mac OS/Mac OS X
and i86 Linux. The source code is available for those wanting 
to do their own ports. 

%prep
%setup -q
cd unix
sed 's/automake --w/automake --add-missing --w/g' -i prebuild.sh
sed 's/dist-bzip2/dist-bzip2 subdir-objects/g' -i configure.ac
./prebuild.sh
cd ..
./bootstrap

%build
./configure LIBS="-lboost_system -lboost_thread -lboost_date_time" COMPILED_BY='Unofficial' --prefix=/usr --sysconfdir=/etc --with-boost-thread=boost_thread
make CXXFLAGS+="-w -lboost_system -lboost_thread -lboost_date_time"

%install
rm -rf %{buildroot}

make DESTDIR=%{buildroot} install
#rm %{buildroot}%{_datadir}/%{name}-3.7/include/.directory

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_docdir}/%{name}-3.7
%{_mandir}/man1/povray.*
%{_datadir}/%{name}-3.7
%{_sysconfdir}/%{name}/3.7

%changelog
* Thu Oct 23 2015 Ole-André Rodlie <olear@fxarena.net> - 3.7.0.0-1
- Updated to final
- Several fixes

* Sat Jun 15 2013 Kim Bisgaard <kim+j2@alleroedderne.adsl.dk> - 3.7.0.RC7-1
- Updated to RC7

* Mon Oct 15 2012 <roma@lcg.ufrj.br> 3.7.0.RC6-1
- Updated to 3.7 RC6.

* Fri Jun 08 2007 <roma@lcg.ufrj.br> 3.6.1-2
- Included missing BRs.

* Wed Aug 02 2006 <roma@lcg.ufrj.br> 3.6.1-1
- Rebuilt for Fedora 5.

* Thu Nov  4 2004 <fenn@stanford.edu> 3.6.1-0
- RPM based off subpop.net FC1 3.50c package
- inc. Wolfgang Wieser's patchset 
  (http://www.cip.physik.uni-muenchen.de/~wwieser/render/povray/patches/)
