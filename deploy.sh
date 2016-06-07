#!/bin/sh
# Build and deploy plugins
#
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
#

CWD=$(pwd)

MAGICK=7.0.1-10
OCIO=1.0.9
OCIO_URL=https://github.com/imageworks/OpenColorIO/archive/v${OCIO}.tar.gz
MAGICK_URL=https://github.com/olear/openfx-arena/releases/download/Natron-2.0.5/ImageMagick-${MAGICK}.tar.xz
if [ -z "$QUANTUM" ]; then
  Q=32
else
  Q=$QUANTUM
fi
MAGICK_OPT="--disable-docs --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=${Q} --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-png --without-rsvg --without-tiff --without-webp --without-xml --without-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --without-fontconfig --without-x --without-modules --without-wmf"

if [ -z "$VERSION" ]; then
  ARENA=2.1.0
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
if [ ! -d $CWD/3rdparty ]; then
  mkdir -p $CWD/3rdparty || exit 1
fi
if [ -z "$JOBS" ]; then
  JOBS=4
fi
OS=$(uname -s)

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

if [ "$OS" = "Linux" ]; then
  PKGOS=Linux
fi
if [ "$OS" = "FreeBSD" ]; then
  PKGOS=FreeBSD
fi
if [ "$OS" = "MINGW64_NT-6.1" ] || [ "$OS" = "MINGW32_NT-6.1" ]; then
  PKGOS=Windows
fi
if [ "$OS" = "Darwin" ]; then
  PKGOS=Mac
fi
if [ "$DEBUG" = "1" ]; then
  TAG=debug
else
  TAG=release
  BF="${BF} -fomit-frame-pointer"
fi

PKG=$PKGNAME.ofx.bundle-$ARENA-$PKGOS-x86-$TAG-$BIT
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export LD_LIBRARY_PATH=$PREFIX/lib:$PREFIX/lib64:$LD_LIBRARY_PATH
export PATH=$PREFIX/bin:$PATH

MAKE=make

if [ "$PKGOS" = "FreeBSD" ]; then
  export CXX=clang++
  export CC=clang
  MAKE=gmake
  BSD="-std=c++11"
  USE_FREEBSD=1
fi

if [ "$CLEAN" = "1" ]; then
  rm -rf $PREFIX || exit 1
fi
if [ ! -d $PREFIX ]; then
  mkdir -p $PREFIX/{bin,lib,include} || exit 1
  (cd $PREFIX ; ln -sf lib lib64)
fi

# magick
if [ "$CLEAN" = "1" ]; then
  rm -rf $CWD/3rdparty/ImageMagick-$MAGICK
fi
if [ ! -f ${PREFIX}/lib/libMagick++-6.Q${Q}HDRI.a ]; then
  if [ "$MAGICK_GIT" = "1" ]; then
    rm -rf $CWD/3rdparty/ImageMagick-6
    cd $CWD/3rdparty || exit 1
    git clone https://github.com/ImageMagick/ImageMagick ImageMagick-6 || exit 1
    cd ImageMagick-6 || exit 1
    git checkout ImageMagick-6 || exit 1
    MAGICK=6
  else
    if [ ! -f $CWD/3rdparty/ImageMagick-$MAGICK.tar.xz ]; then
      wget $MAGICK_URL -O $CWD/3rdparty/ImageMagick-$MAGICK.tar.xz || exit 1
    fi
    if [ ! -d $CWD/3rdparty/ImageMagick-$MAGICK ]; then
      tar xvf $CWD/3rdparty/ImageMagick-$MAGICK.tar.xz -C $CWD/3rdparty/ || exit 1
    fi
    cd $CWD/3rdparty/ImageMagick-$MAGICK || exit 1
  fi
  if [ "$PKGOS" = "Windows" ]; then
    patch -p1 < $CWD/Magick/mingw.patch || exit 1
    patch -p0 < $CWD/Magick/mingw-utf8.diff || exit 1
  fi
  $MAKE distclean
  CFLAGS="-m${BIT} ${BF}" CXXFLAGS="-m${BIT} ${BF} ${BSD} -I${PREFIX}/include" CPPFLAGS="-I${PREFIX}/include -L${PREFIX}/lib" ./configure --libdir=${PREFIX}/lib --prefix=${PREFIX} $MAGICK_OPT || exit 1
  $MAKE -j$JOBS install || exit 1
  mkdir -p $PREFIX/share/doc/ImageMagick/ || exit 1
  cp LICENSE $PREFIX/share/doc/ImageMagick/ || exit 1
  cd .. || exit 1
  if [ "$MAGICK_GIT" != "1" ]; then
    rm -rf ImageMagick-$MAGICK || exit 1
  fi
fi

# ocio
if [ ! -f $PREFIX/lib/libOpenColorIO.a ] && [ "$BUILD_OCIO" = "1" ]; then
    cd $TMP_PATH || exit 1
    if [ ! -f $CWD/3rdparty/OpenColorIO-$OCIO.tar.gz ]; then
        wget $OCIO_URL -O $CWD/3rdparty/OpenColorIO-$OCIO.tar.gz || exit 1
    fi
    tar xvf $CWD/3rdparty/OpenColorIO-$OCIO.tar.gz || exit 1
    cd OpenColorIO-* || exit 1
    rm -rf build
    mkdir build || exit 1
    cd build || exit 1
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${PREFIX}/include" LDFLAGS="-L${PREFIX}/lib" cmake .. -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_BUILD_TYPE=Release -DOCIO_BUILD_SHARED=OFF -DOCIO_BUILD_STATIC=ON -DOCIO_BUILD_APPS=OFF -DOCIO_BUILD_PYGLUE=OFF || exit 1
    make -j${MKJOBS} || exit 1
    make install || exit 1
    cp ext/dist/lib/{liby*.a,libt*.a} $PREFIX/lib/ || exit 1
    sed -i "s/-lOpenColorIO/-lOpenColorIO -lyaml-cpp -ltinyxml -llcms2/" $PREFIX/lib/pkgconfig/OpenColorIO.pc || exit 1
fi

cd $CWD || exit 1

if [ "$PKGNAME" != "Arena" ]; then
  cd $PKGNAME || exit 1
fi

if [ "$STATIC_GCC" = "1" ]; then
  GCC_LINK="-static-libgcc -static-libstdc++"
fi
if [ "$TRAVIS" = "1" ]; then
  TRAVIS_FLAGS="-DLEGACY"
fi
if [ "$PKGOS" != "Windows" ]; then
  $MAKE FREEBSD=$USE_FREEBSD BITS=$BIT LDFLAGS_ADD="$GCC_LINK" CXXFLAGS_ADD="$TRAVIS_FLAGS" CONFIG=$TAG clean
  $MAKE FREEBSD=$USE_FREEBSD BITS=$BIT LDFLAGS_ADD="$GCC_LINK" CXXFLAGS_ADD="$TRAVIS_FLAGS" CONFIG=$TAG || exit 1
else
  make MINGW=1 BIT=$BIT CONFIG=$TAG clean
  make MINGW=1 BIT=$BIT CONFIG=$TAG || exit 1
fi

cd $CWD || exit 1
if [ -d $PKG ]; then
  rm -rf $PKG || exit 1
fi
mkdir $CWD/$PKG || exit 1
cp LICENSE COPYING README.md $CWD/$PKG/ || exit 1
cp $PREFIX/share/doc/ImageMagick/LICENSE $CWD/$PKG/LICENSE.ImageMagick || exit 1
cp OpenFX/Support/LICENSE $CWD/$PKG/LICENSE.OpenFX || exit 1
cp OpenFX-IO/LICENSE $CWD/$PKG/LICENSE.OpenFX-IO || exit 1

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
if [ "$PKGOS" = "Windows" ]; then
  if [ "$TAG" = "release" ]; then
    if [ "$PKGNAME" = "Arena" ]; then
      strip -s Extra/$(uname -s)-$BIT-$TAG/Extra.ofx.bundle/Contents/Win$BIT/*
      strip -s Magick/$(uname -s)-$BIT-$TAG/Magick.ofx.bundle/Contents/Win$BIT/*
    else
      strip -s $PKGSRC/$(uname -s)-$BIT-$TAG/$PKGNAME.ofx.bundle/Contents/Win$BIT/*
    fi
  fi
  if [ "$PKGNAME" = "Arena" ]; then
    mv Extra/$(uname -s)-$BIT-$TAG/Extra.ofx.bundle $CWD/$PKG/ || exit 1
    mv Magick/$(uname -s)-$BIT-$TAG/Magick.ofx.bundle $CWD/$PKG/ || exit 1
  else
    mv $PKGSRC/$(uname -s)-$BIT-$TAG/$PKGNAME.ofx.bundle $CWD/$PKG/ || exit 1
  fi
else
  if [ "$TAG" = "release" ]; then
    if [ "$PKGNAME" = "Arena" ]; then
      strip -s Extra/$(uname -s)-$BIT-$TAG/Extra.ofx.bundle/Contents/$PKGOS-$PKGBIT/Extra.ofx
      strip -s Magick/$(uname -s)-$BIT-$TAG/Magick.ofx.bundle/Contents/$PKGOS-$PKGBIT/Magick.ofx
    else
      strip -s $PKGSRC/$(uname -s)-$BIT-$TAG/$PKGNAME.ofx.bundle/Contents/$PKGOS-$PKGBIT/$PKGNAME.ofx
    fi
  fi
  if [ "$PKGNAME" = "Arena" ]; then
    mv Extra/$(uname -s)-$BIT-$TAG/Extra.ofx.bundle $CWD/$PKG/ || exit 1
    mv Magick/$(uname -s)-$BIT-$TAG/Magick.ofx.bundle $CWD/$PKG/ || exit 1
  else
    mv $PKGSRC/$(uname -s)-$BIT-$TAG/$PKGNAME.ofx.bundle $CWD/$PKG/ || exit 1
  fi
fi

# Package
if [ "$PKGOS" = "Windows" ]; then
  rm -f $PKG.zip
  zip -9 -r $PKG.zip $PKG || exit 1
else
  rm -f $PKG.tgz
  tar cvvzf $PKG.tgz $PKG || exit 1
fi

echo "DONE!!!"
