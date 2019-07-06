# OpenFX-Arena [![Open Hub](https://www.openhub.net/p/openfx-arena/widgets/project_thin_badge?format=gif&ref=Thin+badge)](https://www.openhub.net/p/openfx-arena?ref=Thin+badge) [![Build Status](https://travis-ci.org/NatronGitHub/openfx-arena.svg)](https://travis-ci.org/NatronGitHub/openfx-arena)

A set of [OpenFX](http://openfx.sf.net) plugins (readers, generators, filters, etc) designed for [Natron](https://github.com/NatronGitHub/Natron). Some of the plugins are also compatible with Nuke and DaVinci Resolve.

## Requirements


 * OpenColorIO
 * fontconfig
 * libxml2
 * libzip
 * pangocairo
 * cairo 
 * librsvg2
 * libcdr
 * librevenge
 * poppler-glib
 * lcms2
 * ImageMagick (MagickCore/Magick++)
   * 6.9.10 or 7.0.8
   * Quantum depth 32
   * HDRI
   * OpenMP (optional)
   * lcms2
   * zlib
   * bzip2
   * xz
   * fontconfig
   * freetype
   * libpng
 * OpenCL 1.2 compatible hardware and drivers (optional plugins)
   * Nvidia Kepler (or higher)
   * AMD TeraScale 2 (or higher)
   * Any Intel/AMD CPU using AMD APP SDK


## Build

```
git clone --recurse-submodules https://github.com/NatronGitHub/openfx-arena
mkdir openfx-arena-build && cd openfx-arena-build
cmake -DCMAKE_INSTALL_PREFIX=/usr/OFX/Plugins ../openfx-arena
make -jX
sudo make install
```

