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

 * lcms2 (ReadPSD)
 * OpenColorIO (All Read plugins)
 * POV-Ray (PovRayOFX)
 * fontconfig (TextPango)
 * libxml2 (ReadKRA/ReadORA)
 * libzip (ReadKRA/ReadORA)
 * ImageMagick (6.9.1-10/6.9.2-5+ Q32HDRI) with:
   * Freetype
   * Fontconfig
   * libpng
   * zlib
   * libxml2
   * librsvg
   * lcms2
   * Pangocairo
