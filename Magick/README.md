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

 * ImageMagick 6.9.2+/7.0.1+ with Q32, HDRI, lcms(2), zlib, freetype
 * LittleCMS 2.0+
 * OpenColorIO 1.0.9

Build
=====

Build instructions for Linux, should be similar for BSD, OS X, MinGW.

Install lcms2, zlib, freetype through your package manager, then build a custom version of ImageMagick:

```
sh magick.sh
```

Building Magick.ofx:

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
cd Magick

export PKG_CONFIG_PATH=/usr/local/magick7/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/usr/local/magick7/lib:$LD_LIBRARY_PATH

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

