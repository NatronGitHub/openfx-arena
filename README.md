OpenFX-Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============

A set of [OpenFX](http://openfx.sf.net) plugins designed for [Natron](http://natron.fr).

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
make
```

Requirements:

 * Little CMS v2 (Required by ReadPSD)
 * OpenColorIO (Required by ReadPSD/ReadSVG)
 * ImageMagick (6.9.2-5+ Q32HDRI recommended) with:
   * Freetype (Required by TextOFX)
   * Fontconfig (Required by TextOFX)
   * libpng
   * zlib
   * libxml2
   * librsvg (Required by ReadSVG)
   * lcms2 (Required by ReadPSD)
   * Pangocairo (Required by TextPango)

```
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
