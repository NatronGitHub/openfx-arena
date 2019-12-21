# OpenFX-Arena [![GPL2 License](http://img.shields.io/:license-gpl2-blue.svg?style=flat-square)](https://github.com/NatronGitHub/openfx-arena/blob/master/LICENSE) [![Open Hub](https://www.openhub.net/p/openfx-arena/widgets/project_thin_badge?format=gif&ref=Thin+badge)](https://www.openhub.net/p/openfx-arena?ref=Thin+badge) [![Build Status](https://travis-ci.org/NatronGitHub/openfx-arena.svg)](https://travis-ci.org/NatronGitHub/openfx-arena)

A set of [OpenFX](http://openfx.sf.net) plugins designed for [Natron](http://natron.fr), but also compatible with other OpenFX applications.

## Plugins

 * OpenRaster
 * ReadCDR
 * ReadKrita
 * ReadSVG
 * ReadPDF
 * Text(FX)
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
 * Texture
 * Tile
 * Wave
 * Bulge (OCL)
 * Glare (OCL)
 * Ripple (OCL)
 * Twirl (OCL)
 * Sharpen (OCL)
 * Cartoon (OCL)
 * Duotone (OCL)
 * Edge (OCL)
 * CLFilter (OCL)
 * HaldCLUT
 * Morphology
 * AudioCurve
 * RichText

## Requirements

 * OpenColorIO 1.0.9 *(Extra.ofx/Magic.ofx)*
 * fontconfig *(Extra.ofx/Magick.ofx/Text.ofx)*
 * libxml2 *(OpenRaster/ReadKrita/HaldCLUT)*
 * libzip 1.x *(OpenRaster/ReadKrita)*
 * pango 1.38 *(Text.ofx)*
 * cairo *(Text/ReadCDR/ReadSVG/ReadPDF)*
 * librsvg2 *(ReadCDR/ReadSVG)*
 * libcdr 0.1.x *(ReadCDR)*
 * librevenge *(ReadCDR)*
 * poppler-glib *(ReadPDF)*
 * lcms 2.x *(ReadPSD)*
 * ImageMagick (Magick++) 6.9.2/7.0.3 with Q32, HDRI, lcms2, zlib, freetype, libpng
   * ImageMagick 7.0.8+ recommended
   * Will build on older versions, but some features/plugins may not be available
   * Quantum depth under 32 will work, but it not recommended, note that most distros ship 16
   * Will work without HDRI, but it's not recommended
 * OpenCL 1.2 compatible hardware and drivers (OCL plugins)
 * libcurl *(HaldCLUT)*
 * libsox *(AudioCurve)*

## Build

```
git clone https://github.com/NatronGitHub/openfx-arena
cd openfx-arena
git submodule update -i --recursive
```

### Makefiles

This will build one OFX plugin *(Arena.ofx)*. Only plugins included in the official Natron bundle are built.

Optional options:

  * ``AUDIO=ON``: Enable ``AudioCurve``
  * ``RICHTEXT=ON``: Enable ``RichText``

```
make CONFIG=release
sudo make CONFIG=release install
```

You can also build each "category" as an OFX plugins:

```
make CONFIG=release -C Audio
make CONFIG=release -C Extra
make CONFIG=release -C Magick
make CONFIG=release -C OCL
make CONFIG=release -C Text
sudo cp -a */*-release/*.ofx.bundle /usr/OFX/Plugins/
```

### CMake

This will build one OFX plugin *(Arena.ofx)*. Only plugins included in the official Natron bundle are built.

Optional options:

  * ``-DAUDIO=ON``: Enable ``AudioCurve``
  * ``-DRICHTEXT=ON``: Enable ``RichText``
  * ``-DBUNDLE_FONTS_CONF=ON``: Bundle fonts.conf
  * ``-DMAGICK_PKG_CONFIG=XXX``: Custom Magick++ pkg-config name

```
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/OFX/Plugins ..
sudo make install
```

## License

openfx-arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 (or later) as published by the Free Software Foundation.
