OpenFX-Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============

A set of [OpenFX](http://openfx.sf.net) visual effect plugins designed for [Natron](http://natron.fr).

Plugins
=======

 * **Image**
   * ReadPSD
     * *Read Photoshop/GIMP/Cinepaint RGB/CMYK/GRAY images/layers*
   * ReadSVG
     * *Read SVG vector images*
 * **Filter**
   * Charcoal
     * *Charcoal paint effect*
   * Oilpaint
     * *Oil paint effect*
   * Sketch
     * *Sketch paint effect*
   * Edges
     * *Edge extraction effect*
 * **Transform**
   * Arc
     * *Arc distortion*
   * Polar
     * *Polar distortion*
   * Polaroid
     * *Simulate a polaroid image with text*
   * Reflection
     * *Mirror/Reflects image in various ways*
   * Roll
     * *Roll image*
   * Swirl
     * *Swirl image*
   * Tile
     * *Tile image*
   * Wave
     * *Wave effect*
 * **Draw**
   * Text
     * *Text generator with several options*
   * TextPango
     * *Advanced text generator using Pango markup*
   * Texture
     * *Texture/Background generator*

Source
======

Get latest source:
```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
```

Requirements:

 * Freetype
 * Fontconfig
 * libpng
 * zlib
 * libxml2
 * librsvg
 * liblcms2
 * Pango
 * OpenColorIO
 * ImageMagick (6.8.10-1 **only**, [link](https://github.com/olear/openfx-arena/releases/download/1.9.0/openfx-ImageMagick-6.8.10-1.tar.gz), Q32 with freetype/fontconfig/zlib/pangocairo/rsvg/lcms2/xml2/png)

See 'openfx-arena.spec' or 'deploy.sh' for recommended build options. 

*Builds not using the correct ImageMagick version/options are not supported, and will not function correctly.*

License
=======
```
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
