#!/bin/sh
# Build and deploy plugin
#
# Works on Linux, FreeBSD and Windows(MSYS)
#
# Options:
#
# CLEAN=1 : Clean tmp folder before build
# PACKAGE=foo : Only build one plugin, not bundle
# VERSION=foo : Override package version
#

CWD=$(pwd)

MAGICK=6.8.9-10
MAGICK_URL=https://github.com/olear/openfx-arena/releases/download/0.8.1/ImageMagick-6.8.9-10.tar.gz
if [ -z "$QUANTUM" ]; then
  Q=32
else
  Q=$QUANTUM
fi
if [ "$CL" == "1" ]; then
  CL_CONF="--with-x --enable-opencl"
  CL_FLAGS="-I${CWD}/OpenCL -L${CWD}/OpenCL"
  USE_CL=1
else
  USE_CL=0
fi
MAGICK_OPT="--disable-docs --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=${Q} --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --with-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules $CL_CONF"

PNG=1.2.52
PNG_URL=https://github.com/olear/openfx-arena/releases/download/0.8.1/libpng-1.2.52.tar.gz

if [ -z "$VERSION" ]; then
  ARENA=0.8
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
  BF="-m32 -pipe -O3 -march=i686 -mtune=core2"
  BIT=32
elif [ "$ARCH" = "x86_64" ]; then
  BF="-m64 -pipe -O3 -fPIC -march=core2"
  BIT=64
else
  echo "CPU not supported"
  exit 1
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
if [ "$DEBUG" == "1" ]; then
  TAG=debug
else
  TAG=release
  BF="${BF} -fomit-frame-pointer"
fi

PKG=$PKGNAME.ofx.bundle-$ARENA-$PKGOS-x86-$TAG-$BIT
if [ "$CL" == "1" ]; then
  PKG="${PKG}-OpenCL"
fi

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

# libpng
if [ ! -f ${PREFIX}/lib/libpng.a ] && [ "$OS" != "Msys" ]; then
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
  if [ "$MAGICK" == "6.8.9-10" ]; then
    cat $CWD/3rdparty/composite-private.h > magick/composite-private.h || exit 1
    patch -p0< $CWD/3rdparty/magick-seed.diff || exit 1
    patch -p0< $CWD/3rdparty/magick-svg.diff || exit 1
  fi
  $MAKE distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} ${BSD} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib $CL_FLAGS" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} $MAGICK_OPT || exit 1
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

cd $CWD || exit 1

if [ "$PKGNAME" != "Arena" ]; then
  cd $PKGNAME || exit 1
fi

if [ "$OS" != "Msys" ]; then
  $MAKE USE_SVG=1 OPENCL=$USE_CL USE_PANGO=1 STATIC=1 FREEBSD=$USE_FREEBSD BITS=$BIT CONFIG=$TAG clean
  $MAKE USE_SVG=1 OPENCL=$USE_CL USE_PANGO=1 STATIC=1 FREEBSD=$USE_FREEBSD BITS=$BIT CONFIG=$TAG || exit 1
else
  make STATIC=1 USE_SVG=1 OPENCL=$USE_CL USE_PANGO=1 MINGW=1 BIT=$BIT CONFIG=release clean
  make STATIC=1 USE_SVG=1 OPENCL=$USE_CL USE_PANGO=1 MINGW=1 BIT=$BIT CONFIG=release || exit 1
fi

cd $CWD || exit 1
if [ -d $PKG ]; then
  rm -rf $PKG || exit 1
fi
mkdir $CWD/$PKG || exit 1
cp LICENSE README.md $CWD/$PKG/ || exit 1
cp $PREFIX/share/doc/ImageMagick/LICENSE $CWD/$PKG/LICENSE.ImageMagick || exit 1
cp OpenFX/Support/LICENSE $CWD/$PKG/LICENSE.OpenFX || exit 1
cp OpenFX-IO/LICENSE $CWD/$PKG/LICENSE.OpenFX-IO || exit 1
if [ "$OS" != "Msys" ]; then 
  cp $PREFIX/share/doc/libpng/LICENSE $CWD/$PKG/LICENSE.libpng || exit 1
fi
if [ "$CL" == "1" ]; then
  cp $CWD/OpenCL/LICENSE $CWD/$PKG/LICENSE.OpenCL || exit 1
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
  if [ "$TAG" == "release" ]; then
    strip -s $PKGSRC/$(uname -s)-$BIT-$TAG/$PKGNAME.ofx.bundle/Contents/Win$BIT/*
  fi
  mv $PKGSRC/$(uname -s)-$BIT-$TAG/$PKGNAME.ofx.bundle $CWD/$PKG/ || exit 1
else
  if [ "$TAG" == "release" ]; then
    strip -s $PKGSRC/$(uname -s)-$BIT-$TAG/$PKGNAME.ofx.bundle/Contents/$PKGOS-$PKGBIT/$PKGNAME.ofx || exit 1
  fi
  mv $PKGSRC/$(uname -s)-$BIT-$TAG/$PKGNAME.ofx.bundle $CWD/$PKG/ || exit 1
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
