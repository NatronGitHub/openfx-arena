Summary: A set of OpenFX visual effect plugins
Name: openfx-arena

Version: 2.0.0
Release: 1%{dist}
License: GPLv2

Group: System Environment/Base
URL: https://github.com/olear/openfx-arena

Packager: Ole-André Rodlie, <olear@fxarena.net>
Vendor: FxArena DA, https://fxarena.net

Source: https://github.com/olear/openfx-arena/releases/download/%{version}/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: freetype-devel fontconfig-devel libxml2-devel librsvg2-devel pango-devel zlib-devel OpenColorIO-devel lcms2-devel libpng-devel ImageMagick-c++-devel
Requires: freetype fontconfig libxml2 librsvg2 pango zlib OpenColorIO lcms2 libpng ImageMagick-c++

%description
A set of OpenFX visual effect plugins. Designed for Natron, but should work in other OpenFX hosts.

%prep
%setup

%build
make CONFIG=release
cp OpenFX/Support/LICENSE OpenFX/Support/LICENSE.OpenFX
cp OpenFX-IO/LICENSE OpenFX-IO/LICENSE.OpenFX-IO
cp SupportExt/LICENSE SupportExt/LICENSE.SupportExt

%install
mkdir -p %{buildroot}/usr/OFX/Plugins
cp -a Bundle/Linux-*-release/Arena.ofx.bundle %{buildroot}/usr/OFX/Plugins/
strip -s %{buildroot}/usr/OFX/Plugins/*/*/*/*.ofx

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/OFX/Plugins/Arena.ofx.bundle
%doc README.md COPYING LICENSE OpenFX/Support/LICENSE.OpenFX OpenFX-IO/LICENSE.OpenFX-IO SupportExt/LICENSE.SupportExt

%changelog
