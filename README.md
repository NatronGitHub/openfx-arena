# OpenFX-Arena [![GPL2 License](http://img.shields.io/:license-gpl2-blue.svg?style=flat-square)](https://github.com/NatronGitHub/openfx-arena/blob/master/LICENSE) [![Open Hub](https://www.openhub.net/p/openfx-arena/widgets/project_thin_badge?format=gif&ref=Thin+badge)](https://www.openhub.net/p/openfx-arena?ref=Thin+badge) [![Continuous Integration tests](https://github.com/NatronGitHub/openfx-arena/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/NatronGitHub/openfx-arena/actions/workflows/ci.yml) [![Extended builds](https://github.com/NatronGitHub/openfx-arena/actions/workflows/extended.yml/badge.svg?branch=master)](https://github.com/NatronGitHub/openfx-arena/actions/workflows/extended.yml)

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

 * OpenColorIO 2.x *(the readers, through OpenFX-IO's GenericOCIO; its OpenColorIO 1 code path is not tested)*
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
 * ImageMagick (Magick++) 7, or 6.9.2 or later (older ImageMagick 6 with ``IM=68`` or ``LEGACYIM=1``, below), Q16 or Q32, with HDRI, lcms2, zlib, freetype, libpng. HDRI is required, since the plug-ins pass floating-point pixels to ImageMagick (the build doesn't check it). Natron 2.6 targets ImageMagick 7, static, Q32, HDRI, with OpenMP; ImageMagick 6 HDRI still builds. The Magick effects run ImageMagick on several threads (as many as the host allows) for interactive renders. The other renders, Texture and the readers keep the current limit, usually one thread: the limit only changes when no other ImageMagick call is running. The ``MAGICK_THREAD_LIMIT`` environment variable, when set, overrides this.
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

  * ``AUDIO=ON``: Enable ``AudioCurve`` (the default when pkg-config finds sox; ``AUDIO=OFF`` leaves it out)
  * ``RICHTEXT=ON``: Enable ``RichText`` *(WIP, only for developers)*
  * ``LICENSE=COMMERCIAL``: leave out ``ReadPDF`` and poppler (GPL)
  * ``LEGACYIM=1``: old ImageMagick: leave out ``Sketch``, ``Polaroid`` and ``Edges``, and build without the code that needs a newer ImageMagick (delegate checks, some Text and ReadPSD options, Texture's random seed)
  * ``IM=68``: ImageMagick 6.8: build ``Texture`` without the random seed
  * ``IM=7``: with ``MINGW=1``, also link ``oleaut32`` for ImageMagick 7. Elsewhere it has no effect: the ImageMagick version is detected at compile time
  * ``MINGW=1``: MinGW (MSYS2): add the libraries a static ImageMagick needs (gdi32, freetype, expat, z, ws2_32)
  * ``FREEBSD=1``: FreeBSD: add ``/usr/local/include`` and ``/usr/local/lib`` to the search paths
  * ``MACOSX_DEPLOYMENT_TARGET=XX`` (environment or make variable): the minimum macOS of the bundles, as CMake's ``CMAKE_OSX_DEPLOYMENT_TARGET``; 14.0 by default, as Natron's macOS plug-ins
  * ``LIBOMP_PREFIX=...``: with Apple clang, when ImageMagick was built with OpenMP, the prefix of libomp (``$(LIBOMP_PREFIX)/{include,lib}/libomp``). The default is the ImageMagick prefix if it has ``lib/libomp``, else MacPorts' ``/opt/local``

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

This builds the same *Arena.ofx* bundle as ``make -C Bundle`` (the plug-ins of the official Natron bundle), with CMake 3.21 or later, out of tree. It is assembled in ``build/Plugins``, and installed into ``PLUGIN_INSTALLDIR``: ``OFX/Plugins`` by default, e.g. ``/usr/OFX/Plugins/Arena.ofx.bundle`` with the prefix ``/usr``. A relative ``-DPLUGIN_INSTALLDIR`` is relative to ``CMAKE_INSTALL_PREFIX``; ``-DPLUGIN_INSTALLDIR=.`` gives the previous layout, with the bundle directly in ``CMAKE_INSTALL_PREFIX`` (e.g. ``-DCMAKE_INSTALL_PREFIX=/usr/OFX/Plugins``).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

For an IDE, let CMake generate the project instead of maintaining one: e.g. ``cmake -S . -B build-xcode -G Xcode`` on macOS. On Windows, Arena is built with MSYS2 UCRT64 (MinGW GCC), not MSVC, so use an IDE that opens CMake projects with that toolchain.

Options (the Makefile equivalents are in parentheses):

  * ``-DAUDIO=AUTO|ON|OFF``: ``AudioCurve``; ``AUTO``, the default, builds it if pkg-config finds sox (``AUDIO``)
  * ``-DRICHTEXT=ON``: Enable ``RichText`` *(WIP, only for developers)* (``RICHTEXT=ON``)
  * ``-DOFX_LICENSE=COMMERCIAL``: leave out ``ReadPDF`` and poppler (``LICENSE=COMMERCIAL``)
  * ``-DLEGACYIM=ON``: old ImageMagick, without ``Sketch``, ``Polaroid`` and ``Edges`` (``LEGACYIM=1``)
  * ``-DIM=68``: ImageMagick 6.8: build ``Texture`` without the random seed (``IM=68``). ``-DIM=7``: with MinGW, also link ``oleaut32`` (``IM=7``); elsewhere it has no effect, the ImageMagick version is detected at compile time.
  * ``-DMAGICK_PKG_CONFIG=XXX``: Custom Magick++ pkg-config name (default ``Magick++``, which ImageMagick 6 and 7 both install): only needed to pick one of several ImageMagick installs, e.g. ``Magick++-6.Q16HDRI``. The Makefiles always use ``Magick++``
  * ``-DOFX_USE_OPENMP=OFF``: build without OpenMP (on macOS with Apple clang, OpenMP uses MacPorts' libomp)
  * ``-DOFX_CXX_STANDARD=20``: C++ standard (17 by default; ``ReadPDF`` is always built as C++20)
  * ``-DCMAKE_OSX_DEPLOYMENT_TARGET=XX``: the minimum macOS of the bundles; 14.0 by default, unless the ``MACOSX_DEPLOYMENT_TARGET`` environment variable or a toolchain file sets it (``MACOSX_DEPLOYMENT_TARGET``)
  * ``-DOFX_DEPLOY_BUNDLES=ON``: on macOS, copy the non-system libraries into the installed bundle (Release builds), as the Makefile does
  * ``-DMINGW_STATIC=ON``: link MinGW bundles statically, for hosts other than Natron (``LDFLAGS_ADD=-static``)
  * ``-DHALDCLUT=ON``, ``-DOCL=ON``: also build ``HaldCLUT.ofx.bundle`` and ``OCL.ofx.bundle`` (``make -C Magick/HaldCLUT``, ``make -C OCL``), assembled in ``build/Plugins-HaldCLUT`` and ``build/Plugins-OCL``
  * ``-DBUNDLE_FONTS_CONF=ON``: Bundle fonts.conf *(If not using Natron and on Windows/macOS)*
  * ``-DOLD_POPPLER=ON``: poppler < 0.83
  * ``-DBUILD_TESTING=ON``: ``ctest`` loads the bundles

## License

openfx-arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 (or later) as published by the Free Software Foundation.

Some plug-ins and functions are licensed under the GNU Lesser General Public License 2.1 (or later):

 * Audio/*
 * Midi/*
 * Text/RichText.h
 * Text/RichText.cpp
 * Text/RichTextOFX.cpp
