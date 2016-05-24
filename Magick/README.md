Magick.ofx
==========

OpenFX plugins using ImageMagick.

Plugins
=======

 * Arc
 * Charcoal
 * Edges
 * Implode
 * Modulate
 * Oilpaint
 * Polar
 * Polaroid
 * ReadMisc
 * ReadPSD/XCF
 * Reflection
 * Roll
 * Sketch
 * Swirl
 * Text
 * Texture
 * Tile
 * Wave

Requirements
============

 * ImageMagick 6.9.2+ with Q32, HDRI, lcms(2), zlib, fontconfig, freetype
 * LittleCMS 2.0+
 * OpenColorIO 1.0.9

Build
=====

Build instructions for Linux, should be similar for BSD, OS X, MinGW.

Install lcms2, zlib, freetype, fontconfig through your package manager, then build a custom version of ImageMagick:

```
wget http://imagemagick.org/download/releases/ImageMagick-6.9.4-4.tar.xz
tar xvf ImageMagick-6.9.4-4.tar.xz
cd ImageMagick-6.9.4-4
CPPFLAGS="-fPIC" ./configure --prefix=/usr/local/magick6 --disable-docs --disable-deprecated --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-png --without-rsvg --without-tiff --without-webp --without-xml --with-zlib --without-bzlib --enable-static --disable-shared --enable-hdri --with-freetype --with-fontconfig --without-x --without-modules --without-wmf
make
sudo make install
```

Building Magick.ofx:

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
cd Magick

export PKG_CONFIG_PATH=/usr/local/magick6/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/usr/local/magick6/lib:$LD_LIBRARY_PATH

make CONFIG=release
sudo cp -a Linux-*-*/Magick.ofx.bundle /usr/OFX/Plugins/
```

Compatibility
=============

Magick.ofx has been tested on these hosts:

 * Natron
 * Nuke
 * Vegas

The following plugins has been designed for Natron and may not work as intended on other hosts:

 * ReadPSD/XCF
 * ReadMisc
 * Text
 * Polaroid

