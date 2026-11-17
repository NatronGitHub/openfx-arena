# OpenFX-Arena [![GPL2 License](http://img.shields.io/:license-gpl2-blue.svg?style=flat-square)](https://github.com/NatronGitHub/openfx-arena/blob/master/LICENSE) [![Open Hub](https://www.openhub.net/p/openfx-arena/widgets/project_thin_badge?format=gif&ref=Thin+badge)](https://www.openhub.net/p/openfx-arena?ref=Thin+badge) [![Build Status](https://travis-ci.org/NatronGitHub/openfx-arena.svg)](https://travis-ci.org/NatronGitHub/openfx-arena)

A set of [OpenFX](http://openfx.sf.net) plugins designed for [Natron](https://natrongithub.github.io/).

## Plugins

 * OpenRaster
 * ReadCDR
 * ReadKrita
 * ReadSVG
 * ReadPDF
 * Text
 * Arc
 * Charcoal
 * Edges
 * Implode
 * Modulate
 * Oilpaint
 * Polar
 * Polaroid
 * ReadMisc
 * ReadPSD *(and XCF)*
 * Reflection
 * Roll
 * Sketch
 * Swirl
 * Texture
 * Tile
 * Wave
 * Morphology
 * AudioCurve

Optional *(must be built manually)*:

 * Bulge (OCL)
 * Ripple (OCL)
 * Twirl (OCL)
 * Sharpen (OCL)
 * Cartoon (OCL)
 * Duotone (OCL)
 * Edge (OCL)
 * CLFilter (OCL)
 * HaldCLUT *(similar plugin also included in openfx-gmic)*

Experimental:

 * RichText *(requires modifications to Natron)*
 * MidiIn

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
 * poppler-glib 0.83+ *(ReadPDF)*
 * lcms 2.x *(ReadPSD)*
 * ImageMagick (Magick++) 6.9.2/7.0.3 with Q16, HDRI, lcms2, zlib, freetype, libpng
 * OpenCL 1.2 compatible hardware and drivers *(for optional OCL plugins)*
 * libcurl *(for optional HaldCLUT plugin)*
 * libsox *(AudioCurve)*

## Notes

**Do not use these plugins in DaVinci Resolve or Fusion! Some plugins use OFX features not available in software from BlackMagic Design.**

## Build

```
git clone https://github.com/NatronGitHub/openfx-arena
cd openfx-arena
git submodule update -i --recursive
```

### Makefiles

This will build one OFX plugin bundle *(Arena.ofx)*. Only plugins included in the official Natron bundle are built.

Optional build options:

  * ``AUDIO=ON``: Enable ``AudioCurve``
  * ``RICHTEXT=ON``: Enable ``RichText`` *(WIP, only for developers)*

```
make CONFIG=release
sudo make CONFIG=release install
```

You can also build each "category" as an OFX plugin bundle:

```
make CONFIG=release -C Audio
make CONFIG=release -C Extra
make CONFIG=release -C Magick
make CONFIG=release -C OCL
make CONFIG=release -C Text
sudo cp -a */*-release/*.ofx.bundle /usr/OFX/Plugins/
```

### CMake

This builds the same *Arena.ofx* bundle as ``make -C Bundle`` (the plug-ins of the official Natron bundle), with CMake 3.16 or later, out of tree. It is assembled in ``build/Plugins``, and installed into ``PLUGIN_INSTALLDIR``: ``OFX/Plugins`` by default, e.g. ``/usr/OFX/Plugins/Arena.ofx.bundle`` with the prefix ``/usr``. A relative ``-DPLUGIN_INSTALLDIR`` is relative to ``CMAKE_INSTALL_PREFIX``; ``-DPLUGIN_INSTALLDIR=.`` gives the previous layout, with the bundle directly in ``CMAKE_INSTALL_PREFIX`` (e.g. ``-DCMAKE_INSTALL_PREFIX=/usr/OFX/Plugins``).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

Options (the Makefile equivalents are in parentheses):

  * ``-DAUDIO=AUTO|ON|OFF``: ``AudioCurve``; ``AUTO``, the default, builds it if pkg-config finds sox (``AUDIO``)
  * ``-DRICHTEXT=ON``: Enable ``RichText`` *(WIP, only for developers)* (``RICHTEXT=ON``)
  * ``-DOFX_LICENSE=COMMERCIAL``: leave out ``ReadPDF`` and poppler (``LICENSE=COMMERCIAL``)
  * ``-DLEGACYIM=ON``: old ImageMagick, without ``Sketch``, ``Polaroid`` and ``Edges`` (``LEGACYIM=1``)
  * ``-DIM=68`` or ``-DIM=7``: ImageMagick 6.8 or 7 (``IM=68``, ``IM=7``)
  * ``-DMAGICK_PKG_CONFIG=XXX``: Custom Magick++ pkg-config name
  * ``-DOFX_USE_OPENMP=OFF``: build without OpenMP (on macOS with Apple clang, OpenMP uses MacPorts' libomp)
  * ``-DOFX_CXX_STANDARD=20``: C++ standard (17 by default; ``ReadPDF`` is always built as C++20)
  * ``-DOFX_DEPLOY_BUNDLES=ON``: on macOS, copy the non-system libraries into the installed bundle (Release builds), as the Makefile does
  * ``-DMINGW_STATIC=ON``: link MinGW bundles statically, for hosts other than Natron (``LDFLAGS_ADD=-static``)
  * ``-DHALDCLUT=ON``, ``-DOCL=ON``: also build ``HaldCLUT.ofx.bundle`` and ``OCL.ofx.bundle`` (``make -C Magick/HaldCLUT``, ``make -C OCL``)
  * ``-DBUNDLE_FONTS_CONF=ON``: Bundle fonts.conf *(If not using Natron and on Windows/macOS)*
  * ``-DREADMISC_LEGACY=ON``, ``-DOLD_POPPLER=ON``: legacy formats in ``ReadMisc``, poppler < 0.83
  * ``-DBUILD_TESTING=ON``: ``ctest`` loads the bundles

## License

openfx-arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 (or later) as published by the Free Software Foundation.

Some plug-ins and functions are licensed under the GNU Lesser General Public License 2.1 (or later):

 * Audio/*
 * Midi/*
 * Text/RichText.h
 * Text/RichText.cpp
 * Text/RichTextOFX.cpp
