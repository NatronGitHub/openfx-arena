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

 * Little CMS v2 (lcms2)
 * OpenColorIO
 * ImageMagick 6.9.2+ with:
   * Freetype
   * Fontconfig
   * libpng
   * zlib
   * libxml2
   * librsvg
   * lcms2
   * Pango(Cairo)

See 'openfx-arena.spec' or 'deploy.sh' for recommended build options. 

*Builds not using the correct ImageMagick version/options are not supported, and will not function correctly.*

```
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```
