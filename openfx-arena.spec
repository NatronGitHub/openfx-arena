Summary: A set of OpenFX visual effect plugins
Name: openfx-arena

Version: 2.0.0
Release: 1%{dist}
License: GPLv2

Group: System Environment/Base
URL: https://github.com/olear/openfx-arena

Source: https://github.com/olear/openfx-arena/releases/download/%{version}/%{name}-%{version}.tar.gz
Source1: https://github.com/olear/openfx-arena/releases/download/Natron-2.0.0-RC2/ImageMagick-6.9.1-10.tar.gz
Source2: https://downloads.natron.fr/Third_Party_Sources/OpenColorIO-1.0.9.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires: freetype-devel fontconfig-devel libxml2-devel librsvg2-devel pango-devel zlib-devel lcms2-devel libpng-devel libstdc++-static cmake
Requires: freetype fontconfig libxml2 librsvg2 pango zlib lcms2 libpng

%description
A set of OpenFX visual effect plugins.

%prep
%setup
%setup -T -D -a 1
%setup -T -D -a 2

%build
export ARENA_TMP=$(pwd)/tmp
export PKG_CONFIG_PATH=$ARENA_TMP/lib/pkgconfig
export LD_LIBRARY_PATH=$ARENA_TMP/lib:$LD_LIBRARY_PATH
export PATH=$ARENA_TMP/bin:$PATH

# No distro has the IM version/build we want, so include it
cd ImageMagick-6.9.1-10
patch -p0< ../TextPango/magick-6.9.1-10-pango-align-hack.diff
CFLAGS="-fPIC -O3 " CXXFLAGS="-fPIC -O3" ./configure --prefix=$ARENA_TMP --disable-docs --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --with-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules
make %{?_smp_mflags} install
cp LICENSE LICENSE.ImageMagick
cd ..

# Build our own OCIO, else static gcc won't work (needed for nuke)
cd OpenColorIO-1.0.9
cmake -DCMAKE_INSTALL_PREFIX=$ARENA_TMP -DCMAKE_BUILD_TYPE=Release -DOCIO_BUILD_SHARED=OFF -DOCIO_BUILD_STATIC=ON -DOCIO_BUILD_APPS=OFF -DOCIO_BUILD_PYGLUE=OFF
make %{?_smp_mflags} install
cp ext/dist/lib/{liby*.a,libt*.a} $ARENA_TMP/lib/
sed -i "s/-lOpenColorIO/-lOpenColorIO -lyaml-cpp -ltinyxml -llcms2/" $ARENA_TMP/lib/pkgconfig/OpenColorIO.pc
cp LICENSE LICENSE.OpenColorIO
cd ..

# Build plugins (link static for nuke)
make STATIC=1 CONFIG=release LDFLAGS_ADD="-static-libgcc -static-libstdc++"
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
%doc README.md COPYING LICENSE OpenFX/Support/LICENSE.OpenFX OpenFX-IO/LICENSE.OpenFX-IO SupportExt/LICENSE.SupportExt ImageMagick-6.9.1-10/LICENSE.ImageMagick OpenColorIO-1.0.9/LICENSE.OpenColorIO

%changelog
