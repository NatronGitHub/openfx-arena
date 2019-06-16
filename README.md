OpenFX-Arena [![GPL2 License](http://img.shields.io/:license-gpl2-blue.svg?style=flat-square)](https://github.com/devernay/openfx-arena/blob/master/LICENSE) [![Open Hub](https://www.openhub.net/p/openfx-arena/widgets/project_thin_badge?format=gif&ref=Thin+badge)](https://www.openhub.net/p/openfx-arena?ref=Thin+badge) [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============

A set of [OpenFX](http://openfx.sf.net) plugins designed for [Natron](http://natron.fr) but also compatible with other hosts.

Plugins
=======

 * OpenRaster
 * ReadCDR
 * ReadKrita
 * ReadSVG
 * ReadPDF
 * Text(FX)
 * Arc
 * Charcoal
 * Edges
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
 * Bulge (OCL)
 * Glare (OCL)
 * Ripple (OCL)
 * Twirl (OCL)
 * Sharpen (OCL)
 * Cartoon (OCL)
 * Duotone (OCL)
 * Edge (OCL)
 * CLFilter (OCL)
 * HaldCLUT

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
 * ImageMagick (MagickCore/Magick++) 6.9.2/7.0.3 with Q32, HDRI, lcms2, zlib, freetype, libpng
   * Will build on older versions, but some features/plugins may not be available
   * Quantum depth under 32 will work, but it not recommended, note that most distros ship 16
   * Will work without HDRI, but it's not recommended
 * OpenCL 1.2 compatible hardware and drivers (OCL plugins)
 * libcurl (HaldCLUT)

Build
=====

```
git clone https://github.com/NatronGitHub/openfx-arena
cd openfx-arena
git submodule update -i --recursive
make CONFIG=release IM=7
sudo make CONFIG=release IM=7 install
```

Only plugins included in the official Natron binaries are built, additional plugins can be built using:

```
make CONFIG=release -C OCL
make CONFIG=release IM=7 -C Magick/HaldCLUT
```

License
=======

openfx-arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 (or later) as published by the Free Software Foundation.
