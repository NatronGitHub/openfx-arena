Extra.ofx
=========

Extra OpenFX plugins for Natron.

Plugins
=======

 * OpenRaster
 * ReadCDR
 * ReadKrita
 * ReadSVG
 * TextFX

Requirements
============

 * OpenColorIO
 * fontconfig
 * libxml2
 * libzip
 * pango
 * cairo
 * librsvg2
 * libcdr
 * librevenge

Build
=====

Build instructions for Linux, should be similar for BSD, OS X, MinGW.

Install requirements through your package manager, then build : 

```
git clone https://github.com/olear/openfx-arena
cd openfx-arena
git submodule update -i --recursive
cd Extra

make CONFIG=release
sudo cp -a Linux-*-*/Extra.ofx.bundle /usr/OFX/Plugins/
```

Compatibility
=============

Designed for Natron, but also works in Nuke. Other hosts not tested.

