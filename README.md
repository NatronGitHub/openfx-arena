OpenFX Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena) [![GitHub issues](https://img.shields.io/github/issues/olear/openfx-arena.svg)](https://github.com/olear/openfx-arena/issues)
============

A set of visual effect plugins for OpenFX compatible applications.

Plugins
=======

 * **Swirl**
   * Swirl image
 * **Modulate**
   * Adjust Hue/Saturation/Brightness on image
 * **Mirror**
   * Mirrors image
 * **Implode**
   * Implodes image
 * **Tile**
   * Tile image
 * **Text**
   * Add text to image

Build
=====

Requires Fontconfig and Magick C++ (ImageMagick-Q16 6.5+ or GraphicsMagick-Q16 1.3+) installed prior to build.

**RHEL/Fedora**
```
yum install ImageMagick-c++-devel fontconfig-devel
```

**Debian/Ubuntu**
```
apt-get install libmagick++-dev libfontconfig1-dev
```

**FreeBSD**
```
pkg install ImageMagick fontconfig
```

**Download**
```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i
```

**Make**
```
make CONFIG=release
```

**Install**
```
cp -a Plugin/*-*-*/Arena.ofx.bundle /usr/OFX/Plugins/
```

License
=======
```
Copyright (c) 2015, Ole-André Rodlie <olear@fxarena.net>
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

*  Neither the name of the {organization} nor the names of its
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
