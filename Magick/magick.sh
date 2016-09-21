#!/bin/sh
MAGICK_DIR=${1:-}
if [ -z "$MAGICK_DIR" ]; then
  MAGICK_DIR="/usr/local/magick"
fi
if [ -d "$MAGICK_DIR" ]; then
  sudo rm -rf "$MAGICK_DIR"
fi
if [ -z "$MAGICK_VERSION" ]; then
  MAGICK_VERSION="7.0.3-0"
fi
if [ -z "$MAGICK_JOBS" ]; then
  MAGICK_JOBS=2
fi
if [ ! -f "ImageMagick-${MAGICK_VERSION}.tar.xz" ]; then
  wget http://imagemagick.org/download/releases/ImageMagick-${MAGICK_VERSION}.tar.xz || exit 1
fi
if [ -d "ImageMagick-${MAGICK_VERSION}" ]; then
  rm -rf "ImageMagick-${MAGICK_VERSION}" || exit 1
fi
tar xvf ImageMagick-${MAGICK_VERSION}.tar.xz || exit 1
cd ImageMagick-$MAGICK_VERSION || exit 1
CPPFLAGS="-fPIC" ./configure --prefix=${MAGICK_DIR} --disable-docs --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --with-png --without-rsvg --without-tiff --without-webp --without-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules --without-wmf || exit 1
make -j${MAGICK_JOBS} || exit 1
sudo make install
