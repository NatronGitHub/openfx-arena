Summary: A set of OpenFX visual effect plugins
Name: openfx-arena

Version: 2.0.0
Release: 1%{dist}
License: GPLv2

Group: System Environment/Base
URL: https://github.com/olear/openfx.arena

Packager: Ole-André Rodlie, <olear@fxarena.net>
Vendor: FxArena DA, https://fxarena.net

Source: https://github.com/olear/openfx-arena/releases/download/%{version}/%{name}-%{version}.tar.gz
Source1: https://github.com/olear/openfx-arena/releases/download/1.9.0/openfx-ImageMagick-6.8.10-1.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: freetype-devel fontconfig-devel libxml2-devel librsvg2-devel pango-devel zlib-devel OpenColorIO-devel lcms2-devel libpng-devel
Requires: freetype fontconfig libxml2 librsvg2 pango zlib OpenColorIO lcms2 libpng

%description
A set of OpenFX visual effect plugins. Designed for Natron, but should work in other OpenFX hosts.

%prep
%setup
%setup -T -D -a 1

%build
export ARENA_TMP=$(pwd)/tmp
export PKG_CONFIG_PATH=$ARENA_TMP/lib/pkgconfig
export LD_LIBRARY_PATH=$ARENA_TMP/lib:$LD_LIBRARY_PATH
export PATH=$ARENA_TMP/bin:$PATH
cd ImageMagick-6.8.10-1
CFLAGS="-fPIC -O3 " CXXFLAGS="-fPIC -O3" ./configure --prefix=$ARENA_TMP --disable-docs --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --with-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules 
make %{?_smp_mflags} install
cp LICENSE LICENSE.ImageMagick
cd ..
make USE_SVG=1 USE_PANGO=1 STATIC=1 CONFIG=release
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
%doc README.md COPYING LICENSE ImageMagick-6.8.10-1/LICENSE.ImageMagick OpenFX/Support/LICENSE.OpenFX OpenFX-IO/LICENSE.OpenFX-IO SupportExt/LICENSE.SupportExt

%changelog
