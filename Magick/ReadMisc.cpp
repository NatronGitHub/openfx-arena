/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * openfx-arena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
*/

#include <iostream>
#include <stdint.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"

#define kPluginName "ReadMisc"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadMisc"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 1
#define kPluginEvaluation 93

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsXY false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar false

using namespace OFX::IO;

#ifdef OFX_IO_USING_OCIO
namespace OCIO = OCIO_NAMESPACE;
#endif

OFXS_NAMESPACE_ANONYMOUS_ENTER

class ReadMiscPlugin : public GenericReaderPlugin
{
public:
    ReadMiscPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadMiscPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual bool guessParamsFromFilename(const std::string& filename, std::string *colorspace, OFX::PreMultiplicationEnum *filePremult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
};

ReadMiscPlugin::ReadMiscPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles, kIsMultiPlanar)
{
    Magick::InitializeMagick(NULL);
}

ReadMiscPlugin::~ReadMiscPlugin()
{
}

void
ReadMiscPlugin::decode(const std::string& filename,
                      OfxTime time,
                      int /*view*/,
                      bool /*isPlayback*/,
                      const OfxRectI& renderWindow,
                      float *pixelData,
                      const OfxRectI& bounds,
                      OFX::PixelComponentEnum /*pixelComponents*/,
                      int /*pixelComponentCount*/,
                      int /*rowBytes*/)
{
    #ifdef DEBUG
    std::cout << "decode ..." << std::endl;
    #endif

    Magick::Image image;
    try {
        image.backgroundColor("none"); // must be set to avoid bg
    }
    catch(Magick::Warning &warning) { // ignore since warns interupt render
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
        image.backgroundColor("none"); // must be set to avoid bg
    }
    if (!filename.empty())
        image.read(filename);
    if (image.columns()>0 && image.rows()>0) {
        image.flip();
        image.write(0,0,renderWindow.x2 - renderWindow.x1,renderWindow.y2 - renderWindow.y1,"RGBA",Magick::FloatPixel,pixelData);
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

bool ReadMiscPlugin::getFrameBounds(const std::string& filename,
                                    OfxTime /*time*/,
                                    OfxRectI *bounds,
                                    OfxRectI* format,
                                    double *par,
                                    std::string* /*error*/,int *tile_width, int *tile_height)
{
#ifdef DEBUG
    std::cout << "getFrameBounds ..." << std::endl;
    #endif

    Magick::Image image;
    if (!filename.empty())
        image.ping(filename);
    if (image.columns()>0 && image.rows()>0) {
        bounds->x1 = 0;
        bounds->x2 = image.columns();
        bounds->y1 = 0;
        bounds->y2 = image.rows();
        *format = *bounds;
        *par = 1.0;
    }
    *tile_width = *tile_height = 0;
    return true;
}

bool ReadMiscPlugin::guessParamsFromFilename(const std::string& /*newFile*/,
                                       std::string *colorspace,
                                       OFX::PreMultiplicationEnum *filePremult,
                                       OFX::PixelComponentEnum *components,
                                       int *componentCount)
{
    assert(colorspace && filePremult && components && componentCount);
# ifdef OFX_IO_USING_OCIO
    *colorspace = "sRGB";
# endif
    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st != kOfxStatOK || filename.empty() ) {
        return false;
    }

    Magick::Image image;
    try {
        image.ping(filename);
    }
    catch(Magick::Warning &warning) { // ignore since warns interupt render
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
    }
    if (image.columns()==0 && image.rows()==0) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    *components = OFX::ePixelComponentRGBA;
    *filePremult = OFX::eImageUnPreMultiplied;

    return true;
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadMiscPluginFactory, {}, false);

void
ReadMiscPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("bmp");
    _extensions.push_back("pcx");
    _extensions.push_back("xpm");
    _extensions.push_back("gif");
    _extensions.push_back("miff");
}

/** @brief The basic describe function, passed a plugin descriptor */
void ReadMiscPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    std::string plugCopyright = "\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.";
    # ifdef OFX_IO_USING_OCIO
    plugCopyright.append("\n\nOpenColorIO is Copyright 2003-2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved.\n\nOpenColorIO is distributed under a BSD license.");
    # endif // OFX_IO_USING_OCIO
    desc.setPluginDescription("Read Misc image format.\n\nPowered by "+magickString+plugCopyright);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadMiscPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsXY,kSupportsAlpha, kSupportsTiles, true);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "scene_linear");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadMiscPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadMiscPlugin* ret =  new ReadMiscPlugin(handle, _extensions);
    ret->restoreStateFromParams();
    return ret;
}

static ReadMiscPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
