OpenFX Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena) [![GitHub issues](https://img.shields.io/github/issues/olear/openfx-arena.svg)](https://github.com/olear/openfx-arena/issues)
============

A set of visual effect plugins for OpenFX compatible applications.

Compatibility
=============

Tested on the following hosts:

 * Natron
 * Nuke

Download
========

Download the latest binary bundle from our release page, or download the source:

Stable:

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i
```

Devel:

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git checkout trunk
git submodule update -i
```

Build
=====

Requires MagickCore and Magick++ (Q16-HDRI with fontconfig, freetype, libpng and zlib support, version 6.9.1-4 minimum, or patch lower versions with 'composite-private.h' from the '3rdparty' folder) installed prior to build. Also see 'deploy.sh' for more information.

**Make on Linux**
```
make CONFIG=release
```

**Make on FreeBSD**
```
gmake FREEBSD=1 CONFIG=release
```

**Make on Windows**

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

Contribute
==========

We always need more plugins, if you know ImageMagick (MagickCore/Magick++) please help! Try to avoid plugins that already exists in Natron (openfx-io/openfx-misc) and/or Nuke.

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
