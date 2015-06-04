OpenFX Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena) [![GitHub issues](https://img.shields.io/github/issues/olear/openfx-arena.svg)](https://github.com/olear/openfx-arena/issues)
============

A set of visual effect plugins (OFX) for Natron.

Download
========

Download latest release from:

 * https://github.com/olear/openfx-arena/releases
 * http://openfx.fxarena.net/releases/

Download latest snapshot from:

 * http://openfx.fxarena.net/snapshots/

Build
=====

Requires ImageMagick (MagickCore/Magick++ Q32-HDRI with fontconfig, freetype, libpng and zlib support, version 6.9.1-5) installed prior to build. Also see 'deploy.sh' for more information.

**Make on Linux**
```
make CONFIG=release
```

**Make on FreeBSD**
```
gmake FREEBSD=1 CONFIG=release
```

**Make on Windows (MinGW)**

Download and install MinGW64, MSYS, pkgconfig. Then build zlib, libpng, expat, freetype, fontconfig and ImageMagick. see 'deploy.sh' for recommended build options.

```
make STATIC=1 MINGW=1 BIT=64 CONFIG=release
```

**Install on Linux/FreeBSD**
```
cp -a Bundle/*-*-*/Arena.ofx.bundle /usr/OFX/Plugins/
```

**Install on Windows**
```
cp -a Bundle/*-*-*/Arena.ofx.bundle "/c/Program Files/Common Files/OFX/Plugins/"
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
