#!/bin/sh
CWD=$(pwd)
MAGICK=6.8.9-7
PNG=1.2.52
ARENA=0.2.1
PREFIX=$CWD/tmp
JOBS=4
OS=$(uname -o)

if [ -z "$ARCH" ]; then
  case "$( uname -m )" in
    i?86) export ARCH=i686 ;;
       *) export ARCH=$( uname -m ) ;;
  esac
fi
if [ "$ARCH" = "i686" ]; then
  BF="-pipe -O2 -fomit-frame-pointer -march=i686 -mtune=i686"
  BIT=32
elif [ "$ARCH" = "x86_64" ]; then
  BF="-pipe -O2 -fomit-frame-pointer -fPIC"
  BIT=64
else
  BF="-pipe -O2 -fomit-frame-pointer"
fi

if [ "$OS" == "GNU/Linux" ]; then
  PKGOS=Linux
fi
if [ "$OS" == "FreeBSD" ]; then
  PKGOS=FreeBSD
fi

PKG=Arena.ofx.bundle-$ARENA-$PKGOS-x86-release-$BIT

export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PATH=$PREFIX/bin:$PATH

if [ "$KEEP" != "1" ]; then
  rm -rf $PREFIX
  if [ ! -f $CWD/ImageMagick-$MAGICK.tar.bz2 ]; then
    wget https://github.com/olear/openfx-magick/releases/download/0.1/ImageMagick-$MAGICK.tar.bz2 -O $CWD/ImageMagick-$MAGICK.tar.bz2 || exit 1
  fi
  if [ -d $CWD/ImageMagick-$MAGICK ]; then
    rm -rf $CWD/ImageMagick-$MAGICK || exit 1
  fi
  if [ ! -f $CWD/libpng-$PNG.tar.xz ]; then
    wget http://prdownloads.sourceforge.net/libpng/libpng-$PNG.tar.xz?download -O $CWD/libpng-$PNG.tar.xz || exit 1
  fi
  if [ -d $CWD/libpng-$PNG ]; then
    rm -rf $CWD/libpng-$PNG || exit 1
  fi

  tar xvf libpng-$PNG.tar.xz || exit 1
  cd libpng-$PNG || exit 1
CFLAGS="$BF" CXXFLAGS="$BF -fPIC" ./configure --prefix=$PREFIX --enable-static --disable-shared || exit 1
  make -j$JOBS install || exit 1
  cp LICENSE $PREFIX/LICENSE.png || exit 1
  cd $CWD || exit 1 
  tar xvf ImageMagick-$MAGICK.tar.bz2 || exit 1
  cd ImageMagick-$MAGICK || exit 1
  CFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" ./configure --with-magick-plus-plus=yes --with-quantum-depth=16 --prefix=$PREFIX --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --without-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --with-png --without-rsvg --without-tiff --without-webp --without-xml --without-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules
  make -j$JOBS install
fi

cd $CWD || exit 1
make STATIC=1 CONFIG=release clean
make STATIC=1 CONFIG=release || exit 1

mkdir $CWD/$PKG || exit 1
git log > CHANGELOG || exit 1
cp CHANGELOG LICENSE README.md $CWD/$PKG/ || exit 1
cp $PREFIX/share/doc/ImageMagick-6/LICENSE $CWD/$PKG/LICENSE.ImageMagick || exit 1
cp OpenFX/Support/LICENSE $CWD/$PKG/LICENSE.OpenFX || exit 1
cp $PREFIX/LICENSE.png $CWD/$PKG/ || exit 1
strip -s Plugin/$PKGOS-$BIT-release/Arena.ofx.bundle/Contents/$PKGOS-x86-$BIT/Arena.ofx || exit 1
mv Plugin/$PKGOS-$BIT-release/Arena.ofx.bundle $CWD/$PKG/ || exit 1
tar cvvzf $PKG.tgz $PKG || exit 1

echo "DONE!!!"
