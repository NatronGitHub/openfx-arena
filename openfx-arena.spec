%define debug_package %{nil}
%{!?git_version: %define git_version %(git describe --always --tags|sed 's#-#.#g')}
%{!?git_url: %define git_url https://github.com/olear/openfx-arena}
%{!?git_commit: %define git_commit %(git rev-parse --short HEAD)}

Summary: A set of visual effect plugins for OpenFX compatible applications
Name: openfx-arena

Version: %{git_version}
Release: 1
License: BSD

Group: Applications/Graphics
URL: %{git_url}

Packager: Ole-André Rodlie, <olear@fxarena.net>
Vendor: FxArena DA, http://fxarena.net
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: freetype-devel fontconfig-devel ImageMagick-c++-devel
Requires: freetype fontconfig ImageMagick-c++

%description
A set of visual effect plugins for OpenFX compatible applications.

%prep
cd %{_builddir}
rm -rf openfx-arena-%{version}
git clone %{git_url} openfx-arena-%{version}
cd openfx-arena-%{version}
git checkout %{git_commit}
git submodule update -i
git log > CHANGELOG

%build
cd %{_builddir}/openfx-arena-%{version}
make DEBUGFLAG=-O3

%install
cd %{_builddir}/openfx-arena-%{version}
mkdir -p %{buildroot}/usr/OFX/Plugins %{buildroot}/%{_docdir}/%{name}-%{version}
mv Plugin/*-release/Arena.ofx.bundle %{buildroot}/usr/OFX/Plugins/
cp CHANGELOG LICENSE README.md %{buildroot}/%{_docdir}/%{name}-%{version}/

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_docdir}/*
/usr/OFX/Plugins/*.ofx.bundle

%changelog
#
