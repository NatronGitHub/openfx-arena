OpenFX-Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============

![screenshot-3](https://cloud.githubusercontent.com/assets/7461595/8152563/e60b18c4-131e-11e5-8bd7-6fd6d3dd2db7.png)
A set of [OpenFX](http://openfx.sf.net) visual effect plugins designed for [Natron](http://natron.inria.fr).

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
 * ImageMagick

**Debian/Ubuntu**

See the [Travis](https://github.com/olear/openfx-arena/blob/trunk/.travis.yml) build script.

**RHEL/CentOS/Fedora**

```
yum install freetype-devel fontconfig-devel libxml2-devel librsvg-devel pango-devel zlib-devel OpenColorIO-devel lcms2-devel
```
RHEL/CentOS needs the EPEL repo for OCIO/LCMS support
```
yum install epel-release
```

Now you can run the 'deploy.sh' script in the openfx-arena folder. The script will build libpng and a custom version of ImageMagick, then build the plugins against that. When done you will have a Arena.ofx.bundle-VERSION folder and a TGZ file.

```
sh deploy.sh
```
Replace 'sh' with 'bash' on Debian/Ubuntu.

You can also build manually using 'make'. Please note that then you must have libpng-devel and ImageMagick-devel installed prior to build. Using other version of ImageMagick than the one built using 'deploy.sh' is not supported or recommended.

**Windows**

Can be built using MinGW/MSYS(2), note that depends must be installed before running 'deploy.sh'.

License
=======
```
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.

Contributors must sign CLA or license their work under BSD/MIT for inclusion.
```
