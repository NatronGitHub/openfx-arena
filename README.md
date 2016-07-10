OpenFX-Arena [![Build Status](https://travis-ci.org/olear/openfx-arena.svg)](https://travis-ci.org/olear/openfx-arena)
============

A set of [OpenFX](http://openfx.sf.net) plugins designed for [Natron](http://natron.fr), but also compatible with other hosts.

Extra.ofx
=========

 * OpenRaster
 * ReadCDR
 * ReadKrita
 * ReadSVG
 * ReadPDF
 * Text

[Read more](Extra/README.md)

Magick.ofx
==========

 * Arc
 * Charcoal
 * Edges
 * Implode
 * Modulate
 * Oilpaint
 * Polar
 * Polaroid
 * ReadMisc
 * ReadPSD(XCF)
 * Reflection
 * Roll
 * Sketch
 * Swirl
 * Text (Deprecated)
 * Texture
 * Tile
 * Wave

[Read more](Magick/README.md)

OCL.ofx
=======

 * EdgeCL
 * SharpenCL
 * RippleCL

[Read more](OCL/README.md)

Build
=====

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
make CONFIG=release IM=7
sudo make install
```

License
=======

openfx-arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.
