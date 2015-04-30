#!/bin/sh
# Build and deploy plugin
#
# Works on Linux and Windows
# FreeBSD and Mac is on the TODO
#

CWD=$(pwd)

MAGICK=6.8.9-10 # higher is broken on mingw
MAGICK_URL=ftp://ftp.sunet.se/pub/multimedia/graphics/ImageMagick/ImageMagick-$MAGICK.tar.gz

ZLIB=1.2.8
ZLIB_URL=http://prdownloads.sourceforge.net/libpng/zlib-${ZIB}.tar.gz?download

PNG=1.2.52
PNG_URL=http://prdownloads.sourceforge.net/libpng/libpng-${PNG}.tar.gz?download

EXPAT=2.1.0
EXPAT_URL=http://sourceforge.net/projects/expat/files/expat/${EXPAT}/expat-${EXPAT}.tar.gz/download

FCONFIG=2.10.2
FCONFIG_URL=http://www.freedesktop.org/software/fontconfig/release/fontconfig-${FCONFIG}.tar.gz

FTYPE=2.4.11
FTYPE_URL=http://sourceforge.net/projects/freetype/files/freetype2/${FTYPE}/freetype-${FTYPE}.tar.gz/download

ARENA=0.4
TEXT=1.0
PKGNAME=Text # Arena.ofx crashes against CImg.ofx (probably something stupid I forgot again), only enable MagickText

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

PKG=$PKGNAME.ofx.bundle-$TEXT-$PKGOS-x86-release-$BIT

export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PATH=$PREFIX/bin:$PATH

if [ ! -d $PREFIX ]; then
  mkdir -p $PREFIX/{bin,lib,include} || exit 1
fi

# zlib
if [ ! -f ${PREFIX}/lib/libz.a ] && [ "$OS" == "Msys" ]; then
  if [ ! -d $CWD/3rdparty/zlib-$ZLIB.tar.gz ]; then
    wget $ZLIB_URL -O $CWD/3rdparty/zlib-$ZLIB.tar.gz || exit 1
    tar xvf $CWD/3rdparty/zlib-$ZIB.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/zlib-$ZLIB || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" make -f win32/Makefile.gcc || exit 1
  cp libz.a ${PREFIX}/lib/
  cp *.h ${PREFIX}/include/
fi

# libpng
if [ ! -f ${PREFIX}/lib/libpng.a ]; then
  if [ ! -d $CWD/3rdparty/libpng-$PNG.tar.gz ]; then
    wget $PNG_URL -O $CWD/3rdparty/libpng-$PNG.tar.gz || exit 1
    tar xvf $CWD/3rdparty/libpng-$PNG.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/libpng-$PNG || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --prefix=$PREFIX --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
fi

# expat
if [ ! -f ${PREFIX}/lib/libexpat.a ] && [ "$OS" == "Msys" ]; then
  if [ ! -d $CWD/3rdparty/expat-$EXPAT.tar.gz ]; then
    wget $EXPAT_URL -O $CWD/3rdparty/expat-$EXPAT.tar.gz || exit 1
    tar xvf $CWD/3rdparty/expat-$EXPAT.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/expat-$EXPAT || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
fi

# freetype
if [ ! -f ${PREFIX}/lib/libfreetype.a ] && [ "$OS" == "Msys" ]; then
  if [ ! -d $CWD/3rdparty/freetype-$FTYPE.tar.gz ]; then
    wget $FTYPE_URL -O $CWD/3rdparty/freetype-$FTYPE.tar.gz || exit 1
    tar xvf $CWD/3rdparty/freetype-$FTYPE.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/freetype-$FTYPE || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lexpat -lz" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
fi

# fontconfig
if [ ! -f ${PREFIX}/lib/libfontconfig.a ] && [ "$OS" == "Msys" ]; then
  if [ ! -d $CWD/3rdparty/fontconfig-$FCONFIG.tar.gz ]; then
    wget $FCONFIG_URL -O $CWD/3rdparty/fontconfig-$FCONFIG.tar.gz || exit 1
    tar xvf $CWD/3rdparty/fontconfig-$FCONFIG.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/fontconfig-$FCONFIG || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib -lfreetype -lexpat -lz" ./configure --bindir=${PREFIX} --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-docs --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
fi

# magick
if [ ! -f ${PREFIX}/lib/libMagick++-6.Q16HDRI.a ]; then
  if [ ! -d $CWD/3rdparty/ImageMagick-$MAGICK.tar.gz ]; then
    wget $MAGICK_URL -O $CWD/3rdparty/ImageMagick-$MAGICK.tar.gz || exit 1
    tar xvf $CWD/3rdparty/ImageMagick-$MAGICK.tar.gz -C $CWD/3rdparty/ || exit 1
  fi
  cd $CWD/3rdparty/ImageMagick-$MAGICK || exit 1
  # "backport" from 6.9.1-2
  # prior versions will not produce smooth fonts on transparent rgba, this fixes the problem.
  # why not just use 6.9.1-2? 6.9 is broken on mingw, needs way too many patches, and still wont work
  # 6.8 builds clean, will backport fixes from newer versions if needed.
  cat $CWD/3rdparty/composite-private.h > magick/composite-private.h || exit 1
  make distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=16 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --without-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --with-png --without-rsvg --without-tiff --without-webp --without-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules || exit 1
  make -j$JOBS install || exit 1
fi

cd $CWD || exit 1

if [ "$PKGNAME" != "Arena" ]; then
  cd $PKGNAME || exit 1
fi

if [ "$OS" != "Msys" ]; then
  #probably add gmake for freebsd
  make STATIC=1 CONFIG=release clean
  make STATIC=1 CONFIG=release || exit 1
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
cp $PREFIX/share/doc/ImageMagick-6/LICENSE $CWD/$PKG/LICENSE.ImageMagick || exit 1
cp OpenFX/Support/LICENSE $CWD/$PKG/LICENSE.OpenFX || exit 1
cp $CWD/3rdparty/libpng-*/LICENSE $CWD/$PKG/LICENSE.libpng || exit 1

if [ "$OS" == "Msys" ]; then
  cp $CWD/3rdparty/zlib-*/README $CWD/$PKG/LICENSE.zlib || exit 1
  cp $CWD/3rdparty/expat-*/COPYING $CWD/$PKG/LICENSE.expat || exit 1
  cp $CWD/3rdparty/fontconfig-*/COPYING $CWD/$PKG/LICENSE.fontconfig || exit 1
  cp $CWD/3rdparty/freetype-*/README $CWD/$PKG/LICENSE.freetype || exit 1
fi

# Strip and copy
if [ "$PKGNAME" != "Arena" ]; then
  PKGSRC=$PKGNAME
else
  PKGSRC=Plugin
fi
if [ "$OS" == "Msys" ]; then
  strip -s $PKGSRC/$(uname -s)-$BIT-release/$PKGNAME.ofx.bundle/Contents/Win$BIT/*
  mv $PKGSRC/$(uname -s)-$BIT-release/$PKGNAME.ofx.bundle $CWD/$PKG/ || exit 1
else
  strip -s $PKGSRC/$(uname -s)-$BIT-release/$PKGNAME.ofx.bundle/Contents/$PKGOS-x86-$BIT/$PKGNAME.ofx || exit 1
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
