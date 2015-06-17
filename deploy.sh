#!/bin/sh
# Build and deploy plugin
#
# Works on Linux, FreeBSD and Windows
#
# Options:
#
# CLEAN=1 : Clean tmp folder before build
# MAGICK_BETA=1 : Build ImageMagick snapshot/beta
# MAGICK_STRIP=1 : Build ImageMagick without font stuff
# MAGICK_MOD=1 : Apply ImageMagick mod
# MAGICK_LEGACY=1 : Build against ImageMagick 6.8 (recommended)
# PACKAGE=foo : Only build one plugin, not bundle
# VERSION=foo : Override package version
#

CWD=$(pwd)

MAGICK_LEGACY=1
MAGICK_MOD=1

MAGICK_WIN=6.8.9-10 # higher is broken on mingw
if [ "$MAGICK_LEGACY" == "1" ]; then
  MAGICK_UNIX=$MAGICK_WIN
else
  MAGICK_UNIX=6.9.1-5
fi
MAGICK_UNIX_BETA_MAJOR=6.9.1-6
MAGICK_UNIX_BETA_MINOR=beta20150613
MAGICK_REL_URL=ftp://ftp.sunet.se/pub/multimedia/graphics/ImageMagick
MAGICK_BETA_URL=http://www.imagemagick.org/download/beta
GMAGICK_URL=ftp://ftp.graphicsmagick.org/pub/GraphicsMagick/snapshots
GMAGICK=1.4.020150607
if [ -z "$QUANTUM" ]; then
  Q=32
else
  Q=$QUANTUM
fi
MAGICK_DEF_OPT="--disable-docs --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=${Q} --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --without-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --without-rsvg --without-tiff --without-webp --without-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules"
MAGICK_STRIP_OPT="--disable-docs --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=${Q} --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --without-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-png --without-rsvg --without-tiff --without-webp --without-xml --without-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --without-freetype --without-fontconfig --without-x --without-modules"

if [ "$MAGICK_STRIP" == "1" ]; then
  MAGICK_OPT=$MAGICK_STRIP_OPT
else
  MAGICK_OPT=$MAGICK_DEF_OPT
fi
if [ "$OS" == "Msys" ]; then
  MAGICK=$MAGICK_WIN
else
  if [ "$MAGICK_BETA" == "1" ]; then
    MAGICK="${MAGICK_UNIX_BETA_MAJOR}~${MAGICK_UNIX_BETA_MINOR}"
    MAGICK_URL=$MAGICK_BETA_URL/ImageMagick-$MAGICK.tar.gz
  else
    MAGICK=$MAGICK_UNIX
    MAGICK_URL=$MAGICK_REL_URL/ImageMagick-$MAGICK.tar.gz
  fi
fi
GMAGICK_URL=$GMAGICK_URL/GraphicsMagick-$GMAGICK.tar.gz

ZLIB=1.2.8
ZLIB_URL=http://prdownloads.sourceforge.net/libpng/zlib-${ZLIB}.tar.gz?download

PNG=1.2.52
PNG_URL=http://prdownloads.sourceforge.net/libpng/libpng-${PNG}.tar.gz?download

EXPAT=2.1.0
EXPAT_URL=http://sourceforge.net/projects/expat/files/expat/${EXPAT}/expat-${EXPAT}.tar.gz/download

FCONFIG=2.10.2
FCONFIG_URL=http://www.freedesktop.org/software/fontconfig/release/fontconfig-${FCONFIG}.tar.gz

FTYPE=2.4.11
FTYPE_URL=http://sourceforge.net/projects/freetype/files/freetype2/${FTYPE}/freetype-${FTYPE}.tar.gz/download

FFI=3.2.1
FFI_URL=ftp://sourceware.org/pub/libffi/libffi-$FFI.tar.gz

GLIB=2.42.2
GLIB_URL=http://ftp.acc.umu.se/pub/GNOME/sources/glib/2.42/glib-$GLIB.tar.xz

BUZZ=0.9.40
BUZZ_URL=http://www.freedesktop.org/software/harfbuzz/release/harfbuzz-$BUZZ.tar.bz2

PIX=0.32.6
PIX_URL=http://cairographics.org/releases/pixman-$PIX.tar.gz

CAIRO=1.14.2
CAIRO_URL=http://cairographics.org/releases/cairo-$CAIRO.tar.xz

PANGO=1.37.0
PANGO_URL=http://ftp.gnome.org/pub/GNOME/sources/pango/1.37/pango-$PANGO.tar.xz

if [ -z "$VERSION" ]; then
  ARENA=0.7
else
  ARENA=$VERSION
fi
if [ -z "$PACKAGE" ]; then
  PKGNAME=Arena
else
  PKGNAME=$PACKAGE
fi
if [ -z "$PREFIX" ]; then
  PREFIX=$CWD/tmp
fi
if [ -z "$JOBS" ]; then
  JOBS=4
fi
OS=$(uname -o)

if [ -z "$ARCH" ]; then
  case "$( uname -m )" in
    i?86) export ARCH=i686 ;;
    amd64) export ARCH=x86_64 ;;
       *) export ARCH=$( uname -m ) ;;
  esac
fi
if [ "$ARCH" = "i686" ]; then
  BF="-m32 -pipe -O3 -march=i686 -mtune=i686 -fomit-frame-pointer"
  BIT=32
elif [ "$ARCH" = "x86_64" ]; then
  BF="-m64 -pipe -O3 -fPIC -march=core2 -fomit-frame-pointer"
  BIT=64
else
  BF="-pipe -O2"
fi

if [ "$OS" == "GNU/Linux" ]; then
  PKGOS=Linux
fi
if [ "$OS" == "FreeBSD" ]; then
  PKGOS=FreeBSD
fi
if [ "$OS" == "Msys" ]; then
  PKGOS=Windows
fi

PKG=$PKGNAME.ofx.bundle-$ARENA-$PKGOS-x86-release-$BIT

export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export LD_LIBRARY_PATH=$PREFIX/lib:$PREFIX/lib64:$LD_LIBRARY_PATH
export PATH=$PREFIX/bin:$PATH

MAKE=make

if [ "$OS" == "FreeBSD" ]; then
  export CXX=clang++
  export CC=clang
  MAKE=gmake
  BSD="-std=c++11"
  USE_FREEBSD=1
fi

if [ "$CLEAN" == "1" ]; then
  rm -rf $PREFIX || exit 1
fi
if [ ! -d $PREFIX ]; then
  mkdir -p $PREFIX/{bin,lib,include} || exit 1
  (cd $PREFIX ; ln -sf lib lib64)
fi

# zlib
if [ ! -f ${PREFIX}/lib/libz.a ] && [ "$OS" == "Msys" ]; then
  if [ ! -f $CWD/3rdparty/zlib-$ZLIB.tar.gz ]; then
    wget $ZLIB_URL -O $CWD/3rdparty/zlib-$ZLIB.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/zlib-$ZLIB ]; then
    tar xvf $CWD/3rdparty/zlib-$ZLIB.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/zlib-$ZLIB || exit 1
  if [ -f $CWD/3rdparty/Makefile.zlib ] && [ "$BIT" == "64" ]; then
    cat $CWD/3rdparty/Makefile.zlib > win32/Makefile.gcc || exit 1
  fi
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" make -f win32/Makefile.gcc || exit 1
  cp libz.a ${PREFIX}/lib/
  cp *.h ${PREFIX}/include/
  mkdir -p $PREFIX/share/doc/zlib/ || exit 1
  cp README $PREFIX/share/doc/zlib/ || exit 1
  cd .. || exit 1
  rm -rf zlib-$ZLIB || exit 1
fi

# libpng
if [ ! -f ${PREFIX}/lib/libpng.a ]; then
  if [ ! -f $CWD/3rdparty/libpng-$PNG.tar.gz ]; then
    wget $PNG_URL -O $CWD/3rdparty/libpng-$PNG.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/libpng-$PNG ]; then
    tar xvf $CWD/3rdparty/libpng-$PNG.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/libpng-$PNG || exit 1
  $MAKE distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} ${BSD} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --prefix=$PREFIX --enable-static --disable-shared || exit 1
  $MAKE -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/libpng/ || exit 1
  cp LICENSE $PREFIX/share/doc/libpng/ || exit 1
  cd .. || exit 1
  rm -rf libpng-$PNG || exit 1
fi

# expat
if [ ! -f ${PREFIX}/lib/libexpat.a ]; then
  if [ ! -f $CWD/3rdparty/expat-$EXPAT.tar.gz ]; then
    wget $EXPAT_URL -O $CWD/3rdparty/expat-$EXPAT.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/expat-$EXPAT ]; then
    tar xvf $CWD/3rdparty/expat-$EXPAT.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/expat-$EXPAT || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/expat/ || exit 1
  cp COPYING $PREFIX/share/doc/expat/ || exit 1
  cd .. || exit 1
  rm -rf expat-$EXPAT || exit 1
fi

# freetype
if [ ! -f ${PREFIX}/lib/libfreetype.a ]; then
  if [ ! -f $CWD/3rdparty/freetype-$FTYPE.tar.gz ]; then
    wget $FTYPE_URL -O $CWD/3rdparty/freetype-$FTYPE.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/freetype-$FTYPE ]; then
    tar xvf $CWD/3rdparty/freetype-$FTYPE.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/freetype-$FTYPE || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lexpat -lz" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/freetype/ || exit 1
  cp README $PREFIX/share/doc/freetype/ || exit 1
  cd .. || exit 1
  rm -rf freetype-$FTYPE || exit 1
fi

# fontconfig
if [ ! -f ${PREFIX}/lib/libfontconfig.a ]; then
  if [ ! -f $CWD/3rdparty/fontconfig-$FCONFIG.tar.gz ]; then
    wget $FCONFIG_URL -O $CWD/3rdparty/fontconfig-$FCONFIG.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/fontconfig-$FCONFIG ]; then
    tar xvf $CWD/3rdparty/fontconfig-$FCONFIG.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/fontconfig-$FCONFIG || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lfreetype -lexpat -lz" ./configure --bindir=${PREFIX} --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-docs --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/fontconfig/ || exit 1
  cp COPYING $PREFIX/share/doc/fontconfig/ || exit 1
  cd .. || exit 1
  rm -rf fontconfig-$FCONFIG || exit 1
fi

# libffi
if [ ! -f ${PREFIX}/lib/pkgconfig/libffi.pc ]; then
  if [ ! -f $CWD/3rdparty/libffi-$FFI.tar.gz ]; then
    wget $FFI_URL -O $CWD/3rdparty/libffi-$FFI.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/libffi-$FFI ]; then
    tar xvf $CWD/3rdparty/libffi-$FFI.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/libffi-$FFI || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lfreetype -lexpat -lz" ./configure --bindir=${PREFIX} --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-docs --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/libffi/ || exit 1
  cp COPYING* LIC* $PREFIX/share/doc/libffi/
  cd .. || exit 1
  rm -rf libffi-$FFI || exit 1
fi

# glib
if [ ! -f ${PREFIX}/lib/pkgconfig/glib-2.0.pc ]; then
  if [ ! -f $CWD/3rdparty/glib-$GLIB.tar.xz ]; then
    wget $GLIB_URL -O $CWD/3rdparty/glib-$GLIB.tar.xz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/glib-$GLIB ]; then
    tar xvf $CWD/3rdparty/glib-$GLIB.tar.xz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/glib-$GLIB || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lfreetype -lexpat -lz" ./configure --bindir=${PREFIX} --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-docs --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/glib/ || exit 1
  cp COPYING* LIC* $PREFIX/share/doc/glib/
  cd .. || exit 1
  rm -rf glib-$GLIB || exit 1
fi

# pixman
if [ ! -f ${PREFIX}/lib/pkgconfig/pixman-1.pc ]; then
  if [ ! -f $CWD/3rdparty/pixman-$PIX.tar.gz ]; then
    wget $PIX_URL -O $CWD/3rdparty/pixman-$PIX.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/pixman-$PIX ]; then
    tar xvf $CWD/3rdparty/pixman-$PIX.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/pixman-$PIX || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --bindir=${PREFIX} --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-docs --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/pixman/ || exit 1
  cp COPYING* LIC* $PREFIX/share/doc/pixman/
  cd .. || exit 1
  rm -rf pixman-$PIX || exit 1
fi

# cairo
if [ ! -f ${PREFIX}/lib/pkgconfig/cairo.pc ]; then
  if [ ! -f $CWD/3rdparty/cairo-$CAIRO.tar.xz ]; then
    wget $CAIRO_URL -O $CWD/3rdparty/cairo-$CAIRO.tar.xz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/cairo-$CAIRO ]; then
    tar xvf $CWD/3rdparty/cairo-$CAIRO.tar.xz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/cairo-$CAIRO || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include -I${PREFIX}/include/pixman-1" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lpixman-1" ./configure --bindir=${PREFIX} --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-docs --disable-shared --enable-static || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/cairo/ || exit 1
  cp COPYING* LIC* $PREFIX/share/doc/cairo/
  cd .. || exit 1
  rm -rf cairo-$CAIRO || exit 1
fi

# harfbuzz
if [ ! -f ${PREFIX}/lib/pkgconfig/harfbuzz.pc ]; then
  if [ ! -f $CWD/3rdparty/harfbuzz-$BUZZ.tar.bz2 ]; then
    wget $BUZZ_URL -O $CWD/3rdparty/harfbuzz-$BUZZ.tar.bz2 || exit 1
  fi
  if [ ! -d $CWD/3rdparty/harfbuzz-$BUZZ ]; then
    tar xvf $CWD/3rdparty/harfbuzz-$BUZZ.tar.bz2 -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/harfbuzz-$BUZZ || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lfreetype -lexpat -lz" ./configure --bindir=${PREFIX} --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-docs --enable-static --disable-shared --with-freetype --with-cairo --with-gobject --with-glib --without-icu || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/harfbuzz/ || exit 1
  cp COPYING* LIC* $PREFIX/share/doc/harfbuzz/
  cd .. || exit 1
  rm -rf harfbuzz-$BUZZ || exit 1
  # rebuild freetype
  if [ ! -f $CWD/3rdparty/freetype-$FTYPE.tar.gz ]; then
    wget $FTYPE_URL -O $CWD/3rdparty/freetype-$FTYPE.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/freetype-$FTYPE ]; then
    tar xvf $CWD/3rdparty/freetype-$FTYPE.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/freetype-$FTYPE || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lexpat -lz" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/freetype/ || exit 1
  cp README $PREFIX/share/doc/freetype/ || exit 1
  cd .. || exit 1
  rm -rf freetype-$FTYPE || exit 1
fi

# pango
if [ ! -f ${PREFIX}/lib/pkgconfig/pango.pc ]; then
  if [ ! -f $CWD/3rdparty/pango-$PANGO.tar.xz ]; then
    wget $PANGO_URL -O $CWD/3rdparty/pango-$PANGO.tar.xz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/pango-$PANGO ]; then
    tar xvf $CWD/3rdparty/pango-$PANGO.tar.xz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/pango-$PANGO || exit 1
  make distclean
  FONTCONFIG_CFLAGS="-I${PREFIX}/include" FONTCONFIG_LIBS="-L${PREFIX}/lib -lfontconfig" CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lfontconfig -lfreetype -lexpat -lz" ./configure --bindir=${PREFIX} --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-docs --enable-static --disable-shared --with-included-modules=basic-fc || exit 1
  make -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/pango/ || exit 1
  cp COPYING* LIC* $PREFIX/share/doc/pango/
  cd .. || exit 1
  rm -rf pango-$PANGO || exit 1
fi

# magick
if [ "$CLEAN" == "1" ]; then
  rm -rf $CWD/3rdparty/ImageMagick-$MAGICK
fi
if [ ! -f ${PREFIX}/lib/libMagick++-6.Q${Q}HDRI.a ]; then
  if [ ! -f $CWD/3rdparty/ImageMagick-$MAGICK.tar.gz ]; then
    wget $MAGICK_URL -O $CWD/3rdparty/ImageMagick-$MAGICK.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/ImageMagick-$MAGICK ]; then
    tar xvf $CWD/3rdparty/ImageMagick-$MAGICK.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  if [ "$MAGICK_BETA" == "1" ]; then
    cd $CWD/3rdparty/ImageMagick-$MAGICK_UNIX_BETA_MAJOR || exit 1
  else
    cd $CWD/3rdparty/ImageMagick-$MAGICK || exit 1
  fi
  if [ "$MAGICK_MOD" == "1" ]; then
    cat $CWD/3rdparty/composite-private.h > magick/composite-private.h || exit 1
  fi
  if [ "$MAGICK" == "6.8.9-10" ]; then
    patch -p0< $CWD/3rdparty/magick-seed.diff || exit 1
  fi
  $MAKE distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} ${BSD} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} $MAGICK_OPT || exit 1
  $MAKE -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/ImageMagick/ || exit 1
  cp LICENSE $PREFIX/share/doc/ImageMagick/ || exit 1
  cd .. || exit 1
  if [ "$MAGICK_BETA" == "1" ]; then
    rm -rf ImageMagick-$MAGICK_UNIX_BETA_MAJOR || exit 1
  else
    rm -rf ImageMagick-$MAGICK || exit 1
  fi
fi

# gmagick
if [ "$CLEAN" == "1" ]; then
  rm -rf $CWD/3rdparty/GraphicsMagick-$GMAGICK
fi
if [ ! -f ${PREFIX}/lib/libGraphicsMagick++.a ] && [ "$GM" == "1" ]; then
  if [ ! -f $CWD/3rdparty/GraphicsMagick-$GMAGICK.tar.gz ]; then
    wget $GMAGICK_URL -O $CWD/3rdparty/GraphicsMagick-$GMAGICK.tar.gz || exit 1
  fi
  if [ ! -d $CWD/3rdparty/GraphicsMagick-$GMAGICK ]; then
    tar xvf $CWD/3rdparty/GraphicsMagick-$GMAGICK.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/GraphicsMagick-$GMAGICK || exit 1
  $MAKE distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} ${BSD} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} --with-lcms=no --with-lcms2=no --with-magick-plus-plus=yes --with-tiff=no --with-trio=no --with-jpeg=no --with-jp2=no --with-ttf=no --with-png=no --with-xml=no --with-wmf=no --with-lzma=no --with-bzlib=no --with-zlib=no --with-quantum-depth=${Q} --with-x=no --enable-static --disable-shared || exit 1
  $MAKE -j$JOBS install || exit 1
  cd .. || exit 1
  rm -rf GraphicsMagick-$GMAGICK || exit 1
fi

cd $CWD || exit 1

if [ "$PKGNAME" != "Arena" ]; then
  cd $PKGNAME || exit 1
fi

if [ "$OS" != "Msys" ]; then
  $MAKE USE_PANGO=1 STATIC=1 FREEBSD=$USE_FREEBSD BITS=$BIT CONFIG=release clean
  $MAKE USE_PANGO=1 STATIC=1 FREEBSD=$USE_FREEBSD BITS=$BIT CONFIG=release || exit 1
else
  make STATIC=1 MINGW=1 BIT=$BIT CONFIG=release clean
  make STATIC=1 MINGW=1 BIT=$BIT CONFIG=release || exit 1
fi

cd $CWD || exit 1
if [ -d $PKG ]; then
  rm -rf $PKG || exit 1
fi
mkdir $CWD/$PKG || exit 1
cp LICENSE README.md $CWD/$PKG/ || exit 1
cp $PREFIX/share/doc/ImageMagick/LICENSE $CWD/$PKG/LICENSE.ImageMagick || exit 1
cp OpenFX/Support/LICENSE $CWD/$PKG/LICENSE.OpenFX || exit 1

if [ "$MAGICK_STRIP" != "1" ]; then
  cp $PREFIX/share/doc/libpng/LICENSE $CWD/$PKG/LICENSE.libpng || exit 1
fi

if [ "$OS" == "Msys" ]; then
  cp $PREFIX/share/doc/zlib/README $CWD/$PKG/LICENSE.zlib || exit 1
  cp $PREFIX/share/doc/expat/COPYING $CWD/$PKG/LICENSE.expat || exit 1
  cp $PREFIX/share/doc/fontconfig/COPYING $CWD/$PKG/LICENSE.fontconfig || exit 1
  cp $PREFIX/share/doc/freetype/README $CWD/$PKG/LICENSE.freetype || exit 1
fi

# Strip and copy
if [ "$PKGNAME" != "Arena" ]; then
  PKGSRC=$PKGNAME
else
  PKGSRC=Bundle
fi
if [ "$BIT" == "64" ]; then
  PKGBIT=x86-$BIT
else
  PKGBIT=x86
fi
if [ "$OS" == "Msys" ]; then
  strip -s $PKGSRC/$(uname -s)-$BIT-release/$PKGNAME.ofx.bundle/Contents/Win$BIT/*
  mv $PKGSRC/$(uname -s)-$BIT-release/$PKGNAME.ofx.bundle $CWD/$PKG/ || exit 1
else
  strip -s $PKGSRC/$(uname -s)-$BIT-release/$PKGNAME.ofx.bundle/Contents/$PKGOS-$PKGBIT/$PKGNAME.ofx || exit 1
  mv $PKGSRC/$(uname -s)-$BIT-release/$PKGNAME.ofx.bundle $CWD/$PKG/ || exit 1
fi

# Package
if [ "$OS" == "Msys" ]; then
  rm -f $PKG.zip
  zip -9 -r $PKG.zip $PKG || exit 1
else
  rm -f $PKG.tgz
  tar cvvzf $PKG.tgz $PKG || exit 1
fi

echo "DONE!!!"
