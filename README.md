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

 * Little CMS v2 (Required by ReadPSD)
 * OpenColorIO (Required by ReadPSD/ReadSVG)
 * POV-Ray (runtime by PovRayOFX)
 * fontconfig (Required by TextPango)
 * ImageMagick (6.9.1-10/6.9.2-5+ Q32HDRI recommended) with:
   * Freetype (Required by TextOFX)
   * Fontconfig (Required by TextOFX/TextPango/Polaroid)
   * libpng
   * zlib
   * libxml2
   * librsvg (Required by ReadSVG)
   * lcms2 (Required by ReadPSD)
   * Pangocairo (Required by TextPango)
