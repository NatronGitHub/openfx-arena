# OpenFX-Arena [![Open Hub](https://www.openhub.net/p/openfx-arena/widgets/project_thin_badge?format=gif&ref=Thin+badge)](https://www.openhub.net/p/openfx-arena?ref=Thin+badge) [![Build Status](https://travis-ci.org/NatronGitHub/openfx-arena.svg)](https://travis-ci.org/NatronGitHub/openfx-arena)

A set of [OpenFX](http://openfx.sf.net) image readers, generators and effects for [Natron](https://github.com/NatronGitHub/Natron).

## Features

 * Read Inkscape/SVG documents and layers *(SVG)*
 * Read MyPaint/OpenRaster images and layers *(ORA)*
 * Read Krita images and layers *(KRA)*
 * Read GIMP images and layers *(XCF)*
 * Read Adobe Photoshop images and layers *(PSD)*
 * Read CorelDRAW documents *(CDR)*
 * Read PDF documents *(PDF)*
 * Advanced text generator(s) using Pango and Cairo
 * Various image effects using OpenCL
 * Various image effects using ImageMagick

## Requirements

 * OpenFX host
   * Natron 2.x
   * Nuke 7+ *(some plugins may not work)*
   * DaVinci Resolve 15+ *(some plugins may not work)*
 * cmake *(3.1, optional)*
 * OpenColorIO
 * fontconfig
 * libxml2
 * libzip
 * pangocairo
 * cairo 
 * librsvg2
 * libcdr
 * librevenge
 * poppler-glib
 * lcms2
 * ImageMagick (*Magick++)*
   * 6.9.10 or 7.0.8
   * Quantum depth 32
   * HDRI
   * OpenMP *(optional)*
   * lcms2
   * zlib
   * bzip2
   * xz
   * fontconfig
   * freetype
   * libpng
 * OpenCL 1.2 compatible hardware and drivers *(optional)*
   * Nvidia Kepler *(or higher)*
   * AMD TeraScale 2 *(or higher)*
   * Any Intel/AMD CPU using AMD APP SDK


## Build and install (Linux)

```
git clone --recurse-submodules https://github.com/NatronGitHub/openfx-arena
mkdir openfx-arena-build && cd openfx-arena-build
cmake -DCMAKE_INSTALL_PREFIX=/usr/OFX/Plugins ../openfx-arena
make -jX
sudo make install
```

*Regular Makefiles are also available.*

## Contribute

Do you known ImageMagick (Magick++)? or are you able to write OpenCL kernels? Then you can definitely contribute.

## Creating a new ImageMagick plugin

We will start with a skeleton based on the ``MagickPlugin`` template.

```
#include "MagickPlugin.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

// Generic plugin name
#define kPluginName "Flip"

// Plugin category (and subcategory)
// ImageMagick plugins are usually located in 'Extra'
#define kPluginGrouping "Extra/Transform"

// Unique identifier, should start with 'org.imagemagick.'
#define kPluginIdentifier "org.imagemagick.Flip"

// A short description of your plugin
#define kPluginDescription "Flips the image using ImageMagick."

// Plugin version
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

class Flip
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    Flip(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
    {
    }

    virtual void render(const OFX::RenderArguments &args,
                        Magick::Image &image) OVERRIDE FINAL
    {
        // Let's just flip the image using Magick++
        image.flip(true);
    }
};

mDeclarePluginFactory(FlipFactory, {}, {});

void FlipFactory::describe(ImageEffectDescriptor &desc)
{
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);
    desc.addSupportedBitDepth(eBitDepthFloat);
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
    desc.setHostMaskingEnabled(kHostMasking);
    desc.setHostMixingEnabled(kHostMixing);
}

void FlipFactory::describeInContext(ImageEffectDescriptor &desc,
                                    ContextEnum context)
{
    Flip::describeInContextEnd(desc, context, page);
}

ImageEffect* FlipFactory::createInstance(OfxImageEffectHandle handle,
                                         ContextEnum /*context*/)
{
    return new Flip(handle);
}

static FlipFactory p(kPluginIdentifier,
                     kPluginVersionMajor,
                     kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
```

That's the basics for a simple plugin using ImageMagick. For more advanced usage take a look at other ImageMagick plugins included in this repository.

A good reference for ImageMagick documentation can be found at:

* http://www.imagemagick.org/Magick++/Image++.html
* http://www.imagemagick.org/Usage/

### Creating OpenCL plugins

The process is more or less the same, but you use the ``OCLPlugin`` template instead. More documentation will be added in the future. It's recommended to use the ``OCLFilter`` plugin to test kernels. As always it's recommended to look at existing plugins when you develop your own.

## About

Originally written by Ole-André Rodlie for Natron and INRIA.

Maintained by Frédéric Devernay and Ole-André Rodlie.

Distributed under the GNU General Public License version 2.0 or later.