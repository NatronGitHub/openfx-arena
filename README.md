OpenFX Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============

A set of visual effect plugins for OpenFX compatible applications.

Plugins
=======
 * **Swirl**
   * Swirl image
 * **Text**
   * Add text to image [WIP]

Build
=====

Requires FreeType, FontConfig and ImageMagick installed prior to build.

**RHEL/Fedora:**
```
yum install ImageMagick-c++-devel freetype-devel fontconfig-devel
```

**Debian/Ubuntu:**
```
apt-get install libmagick++-dev libfreetype6-dev libfontconfig1-dev
```

**Build ImageMagick from source:** (optional)
```
CFLAGS="-pipe -O2 -fomit-frame-pointer -fPIC -msse -msse2 -msse3 -mmmx -m3dnow -march=core2" CXXFLAGS="-pipe -O2 -fomit-frame-pointer -fPIC -msse -msse2 -msse3 -mmmx -m3dnow -march=core2" ./configure --with-magick-plus-plus=yes --with-quantum-depth=16 --prefix=$IM --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --without-lcms --without-lcms2 --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --without-png --without-rsvg --without-tiff --without-webp --without-xml --without-zlib --without-bzlib --enable-static --disable-shared --with-x --enable-hdri --with-freetype --with-fontconfig
```
OpenCL support:

Download and install OpenCL headers and libs, then add :
```
--enable-opencl
```
at the end of the configure.

**Build and install OpenFX Arena:**
```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i
make DEBUGFLAG=-O3
cp -a Plugin/*-release/Arena.ofx.bundle /usr/OFX/Plugins/
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
