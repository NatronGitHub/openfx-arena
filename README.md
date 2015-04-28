OpenFX Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena) [![GitHub issues](https://img.shields.io/github/issues/olear/openfx-arena.svg)](https://github.com/olear/openfx-arena/issues)
============

A set of visual effect plugins for OpenFX compatible applications.


Compatibility
=============

 * Nuke
 * Natron #1
 * Vegas #15

Download
========

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
```

Or download from the releases page.

Build
=====

Requires Fontconfig and Magick C++ (Q16-HDRI, 6.8+ recommended) installed prior to build.

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

**Windows**

  * https://fxarena.net/~olear/misc/mingw64.7z
  * https://fxarena.net/~olear/misc/MSYS-20111123.zip
  * https://fxarena.net/~olear/misc/local.7z
  * 
  
Extract mingw64.7z and MSYS.zip to C:, then extract local.7z to C:/msys/

Start MSYS from C:/msys/msys.bat.

Run post install
```
sh /postinstall/pi.sh
```

 * Type "y" to continue with the post install
 * Type "y" to say that MinGW is installed
 * Enter "c:/mingw64" as the MinGW installation location

**Make on Linux/BSD**
```
make CONFIG=release
```

**Make on Windows/MinGW**
```
export PATH=/usr/local/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

make STATIC=1 MINGW=1 BIT=64 CONFIG=release
```

**Install on Linux/BSD**
```
cp -a Plugin/*-*-*/Arena.ofx.bundle /usr/OFX/Plugins/
```

**Install on Windows**
```
cp -a Plugin/*-*-*/Arena.ofx.bundle "/c/Program Files/Common Files/OFX/Plugins/"
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
