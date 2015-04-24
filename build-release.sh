#!/bin/sh
CWD=$(pwd)
MAGICK=6.8.9-7
ARENA=0.2
PREFIX=$CWD/tmp
JOBS=4
PKG=Arena.ofx.bundle-$ARENA-Linux-x86-release-64

if [ ! -f $CWD/ImageMagick-$MAGICK.tar.bz2 ]; then
  wget https://github.com/olear/openfx-magick/releases/download/0.1/ImageMagick-$MAGICK.tar.bz2
fi
if [ -d $CWD/ImageMagick-$MAGICK ]; then
  rm -rf $CWD/ImageMagick-$MAGICK || exit 1
fi

if [ "$KEEP_MAGICK" != "1" ]; then
  tar xvf ImageMagick-$MAGICK.tar.bz2 || exit 1
  cd ImageMagick-$MAGICK || exit 1

  CFLAGS="-pipe -O2 -fomit-frame-pointer -fPIC -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="-pipe -O2 -fomit-frame-pointer -fPIC -DMAGICKCORE_EXCLUDE_DEPRECATED=1" ./configure --with-magick-plus-plus=yes --with-quantum-depth=16 --prefix=$PREFIX --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --without-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-png --without-rsvg --without-tiff --without-webp --without-xml --without-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules
make -j$JOBS install
fi

cd $CWD || exit 1
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
export LD_LIBRARY_PATH=$PREFIX/lib:$LD_LIBRARY_PATH
export PATH=$PREFIX/bin:$PATH
make STATIC=1 CONFIG=release clean
make STATIC=1 CONFIG=release

mkdir $CWD/$PKG || exit 1

git log > CHANGELOG || exit 1
cp CHANGELOG LICENSE README.md $CWD/$PKG/ || exit 1
cp $PREFIX/share/doc/ImageMagick-6/LICENSE $CWD/$PKG/LICENSE.ImageMagick || exit 1
cp OpenFX/Support/LICENSE $CWD/$PKG/LICENSE.OpenFX || exit 1
mv Plugin/Linux-64-release/Arena.ofx.bundle $CWD/$PKG/ || exit 1

tar cvvzf $PKG.tgz $PKG || exit 1


