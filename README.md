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
 * TextFX

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
 * Text
 * Texture
 * Tile
 * Wave

[Read more](Magick/README.md)

Build
=====

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
make LICENSE=GPL CONFIG=release IM=7
sudo make install
```
