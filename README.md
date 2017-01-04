OpenFX-Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============

A set of [OpenFX](http://openfx.sf.net) plugins designed for [Natron](http://natron.fr) but also compatible with other hosts.

Plugins
=======

 * OpenRaster
 * ReadKrita
 * ReadSVG(PDF/CDR)
 * Text
 * Arc
 * Charcoal
 * Implode
 * Modulate
 * Oilpaint
 * Polar
 * Polaroid
 * ReadMisc
 * ReadPSD(XCF)
 * Reflection
 * Roll
 * Sketch
 * Swirl
 * Texture
 * Tile
 * Wave
 * Bulge (wip)
 * Glare (wip)
 * Ripple
 * Twirl (wip)
 * Duotone

Requirements
============

 * OpenColorIO 1.0.9
 * fontconfig
 * libxml2
 * libzip 1.x
 * pango 1.38
 * cairo 
 * librsvg2
 * libcdr 0.1.x
 * librevenge
 * poppler-glib
 * lcms 2.x
 * ImageMagick (MagickCore/Magick++) 6.9.2/7.0.3 with Q16/Q32, HDRI, lcms2, zlib, freetype, libpng
 * OpenCL 1.1 compatible hardware and drivers


Build
=====

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
make CONFIG=release IM=7
sudo make CONFIG=release IM=7 install
```

License
=======

openfx-arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.
