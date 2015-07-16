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

Downloads
=========

See latest [release](https://github.com/olear/openfx-arena/releases) for binary downloads.

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

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Neither the name of FxArena DA nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
