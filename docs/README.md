# OpenFX-Arena [![Open Hub](https://www.openhub.net/p/openfx-arena/widgets/project_thin_badge?format=gif&ref=Thin+badge)](https://www.openhub.net/p/openfx-arena?ref=Thin+badge) [![Build Status](https://travis-ci.org/NatronGitHub/openfx-arena.svg)](https://travis-ci.org/NatronGitHub/openfx-arena)

A set of [OpenFX](http://openfx.sf.net) image readers, generators and effects for [Natron](https://github.com/NatronGitHub/Natron).

## Features

 * Read Inkscape/SVG documents and layers *(SVG)*
 * Read MyPaint/OpenRaster images and layers *(ORA)*
 * Read Krita images and layers *(KRA)*
 * Read GIMP images and layers *(XCF)*
 * Read Adobe Photoshop images and layers *(PSD)*
 * Read CorelDRAW documents *(CDR)*
 * Read PDF documents *(PDF)*
 * Advanced text generator(s) using Pango and Cairo
 * Various image effects using OpenCL
 * Various image effects using ImageMagick

## Requirements

 * OpenFX host
   * Natron 2.x
   * Nuke 7+ *(some plugins may not work)*
   * DaVinci Resolve 15+ *(some plugins may not work)*
 * cmake *(3.1, optional)*
 * OpenColorIO
 * fontconfig
 * libxml2
 * libzip
 * pangocairo
 * cairo 
 * librsvg2
 * libcdr
 * librevenge
 * poppler-glib
 * lcms2
 * ImageMagick (*Magick++)*
   * 6.9.4+ or 7.0.0+
   * Quantum depth 32
   * HDRI
   * OpenMP *(optional)*
   * lcms2
   * zlib
   * bzip2
   * xz
   * fontconfig
   * freetype
   * libpng
 * OpenCL 1.2 compatible hardware and drivers *(optional)*
   * Nvidia Kepler *(or higher)*
   * AMD TeraScale 2 *(or higher)*
   * Any Intel/AMD CPU using AMD APP SDK


## Build and install (Linux)

```
git clone --recurse-submodules https://github.com/NatronGitHub/openfx-arena
mkdir openfx-arena-build && cd openfx-arena-build
cmake -DCMAKE_INSTALL_PREFIX=/usr/OFX/Plugins ../openfx-arena
make -jX
sudo make install
```

*Regular Makefiles are also available.*

## About

Originally written by Ole-André Rodlie for Natron and INRIA.

Maintained by Frédéric Devernay and Ole-André Rodlie.

Distributed under the GNU General Public License version 2.0 or later.
