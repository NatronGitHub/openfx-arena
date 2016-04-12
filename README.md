OpenFX-Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============
![logo](https://raw.githubusercontent.com/olear/openfx-arena/trunk/Bundle/Extra.png)

A set of extra [OpenFX](http://openfx.sf.net) plugins for [Natron](http://natron.fr) VFX.

Plugins
=======

 * Arc
 * Charcoal
 * Edges
 * Implode
 * Modulate
 * Oilpaint
 * OpenRaster
 * Polar
 * Polaroid
 * PovRay
 * ReadCDR
 * ReadKrita
 * ReadMisc
 * ReadPSD
 * ReadSVG
 * Reflection
 * Roll
 * Sketch
 * Swirl
 * Text (EOL)
 * TextFX
 * TextPango (EOL)
 * Texture
 * Tile
 * Wave

Requirements
============

 * lcms2
 * OpenColorIO
 * POV-Ray (runtime)
 * fontconfig
 * libxml2
 * libzip
 * librevenge
 * libcdr
 * librsvg2
 * pango
 * cairo
 * ImageMagick (6.9.3-5 Q32HDRI) with:
   * freetype
   * fontconfig
   * libpng
   * zlib
   * lcms2
   * pango
   * cairo
   * libxml2
   
 Note that custom patches to ImageMagick is needed to build the plugins, see 'deploy.sh' for more information.

Build
=====

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
make
```

Also see  'deploy.sh' and 'openfx-arena.spec'.

Compatibility
=============

 * All plugin work in Natron
 * Most of the plugins work in Nuke
 * Some of the plugins work(ed) in Shake

Contribute
==========

You are welcome to contribute, please fork and do a pull request. Note that major contributions may require a CLA.

Contributors
============

 * Ole-André Rodlie (FxArena/INRIA)
 * Alexandre Gauthier-Foichat (INRIA)
 * John-Åge Width Olsen (FxArena)

License
=======

All plugins are licensed under the GPL version 2.
