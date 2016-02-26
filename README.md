OpenFX-Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============
![logo](https://raw.githubusercontent.com/olear/openfx-arena/trunk/Bundle/Extra.png)

A set of extra [OpenFX](http://openfx.sf.net) plugins for [Natron](http://natron.fr) VFX.

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
make
```

Plugins:

 * Arc
 * Charcoal
 * Edges
 * Emboss (wip)
 * Filmstrip (wip)
 * Implode
 * Modulate
 * Oilpaint
 * OpenRaster
 * Polar
 * Polaroid
 * PovRay
 * ReadCDR (wip)
 * ReadPDF (wip)
 * ReadKrita
 * ReadMisc
 * ReadPSD
 * ReadSVG
 * ReadVisio (wip)
 * Reflection
 * Roll
 * Sketch
 * Swirl
 * Text
 * TextPango
 * TextFX (wip)
 * Texture
 * Tile
 * Wave
 * WaveletDenoise (wip)

Requirements:

 * lcms2
 * OpenColorIO
 * POV-Ray (runtime)
 * fontconfig
 * libxml2
 * libzip
 * librevenge (wip)
 * libcdr (wip)
 * libvisio (wip)
 * pangocairo (wip)
 * ImageMagick (6.9.3-6+ Q32HDRI) with:
   * freetype
   * fontconfig
   * libpng
   * zlib
   * libxml2
   * librsvg
   * lcms2
   * pango
   * cairo
   * ps/ghostscript (wip)
   
 Note that custom patches to ImageMagick is needed to build the plugins, see 'deploy.sh' for more information.
