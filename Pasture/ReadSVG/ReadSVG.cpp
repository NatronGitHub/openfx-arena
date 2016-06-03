/*
# Copyright (c) 2015, Ole-André Rodlie <olear@dracolinux.org>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
*/

#include <iostream>
#include <stdint.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadSVG"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadSVG"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 7
#define kPluginEvaluation 50

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch (90 is default)"
#define kParamDpiDefault 90

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

class ReadSVGPlugin : public GenericReaderPlugin
{
public:
    ReadSVGPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadSVGPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error,int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    OFX::IntParam *dpi_;
    bool hasRSVG_;
};

ReadSVGPlugin::ReadSVGPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
,dpi_(0)
,hasRSVG_(false)
{
    Magick::InitializeMagick(NULL);
    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("rsvg") != std::string::npos)
        hasRSVG_ = true;

    dpi_ = fetchIntParam(kParamDpi);
    assert(dpi_);
}

ReadSVGPlugin::~ReadSVGPlugin()
{
}

void
ReadSVGPlugin::decode(const std::string& filename,
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

    if (!hasRSVG_)
        setPersistentMessage(OFX::Message::eMessageError, "", "librsvg missing, some features may not work as expected");
    int dpi = 0;
    dpi_->getValueAtTime(time, dpi);
    Magick::Image image;
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
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
        Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,1)"));
        container.composite(image,0,0,Magick::OverCompositeOp);
        container.composite(image,0,0,Magick::CopyOpacityCompositeOp);
        container.flip();
        container.write(0,0,renderWindow.x2 - renderWindow.x1,renderWindow.y2 - renderWindow.y1,"RGBA",Magick::FloatPixel,pixelData);
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

bool ReadSVGPlugin::getFrameBounds(const std::string& filename,
                              OfxTime time,
                              OfxRectI *bounds,
                              double *par,
                              std::string* /*error*/,int *tile_width, int *tile_height)
{
    #ifdef DEBUG
    std::cout << "getFrameBounds ..." << std::endl;
    #endif

    int dpi;
    dpi_->getValueAtTime(time, dpi);
    Magick::Image image;
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    if (!filename.empty())
        image.ping(filename);
    if (image.columns()>0 && image.rows()>0) {
        bounds->x1 = 0;
        bounds->x2 = image.columns();
        bounds->y1 = 0;
        bounds->y2 = image.rows();
        *par = 1.0;
    }
    *tile_width = *tile_height = 0;
    return true;
}

void ReadSVGPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    #ifdef DEBUG
    std::cout << "onInputFileChanged ..." << std::endl;
    #endif

    assert(premult && components);
    int dpi;
    dpi_->getValue(dpi);
    Magick::Image image;
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    try {
        image.ping(newFile);
    }
    catch(Magick::Warning &warning) { // ignore since warns interupt render
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
    }
    if (image.columns()>0 && image.rows()>0) {
        if (setColorSpace) {
        # ifdef OFX_IO_USING_OCIO
            switch(image.colorSpace()) {
            case Magick::RGBColorspace:
                _ocio->setInputColorspace("sRGB");
                break;
            case Magick::sRGBColorspace:
                _ocio->setInputColorspace("sRGB");
                break;
            case Magick::scRGBColorspace:
                _ocio->setInputColorspace("sRGB");
                break;
            default:
                _ocio->setInputColorspace("Linear");
                break;
            }
        # endif // OFX_IO_USING_OCIO
        }
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageOpaque;
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadSVGPluginFactory, {}, false);

void
ReadSVGPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("svg");
}


/** @brief The basic describe function, passed a plugin descriptor */
void ReadSVGPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    std::string plugCopyright = "\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.";
    # ifdef OFX_IO_USING_OCIO
    plugCopyright.append("\n\nOpenColorIO is Copyright 2003-2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved.\n\nOpenColorIO is distributed under a BSD license.");
    # endif // OFX_IO_USING_OCIO
    desc.setPluginDescription("Read SVG image format.\n\nPowered by "+magickString+plugCopyright);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadSVGPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false);
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamDpi);
        param->setLabel(kParamDpiLabel);
        param->setHint(kParamDpiHint);
        param->setRange(1, 10000);
        param->setDisplayRange(1, 500);
        param->setDefault(kParamDpiDefault);
        param->setAnimates(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadSVGPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadSVGPlugin* ret =  new ReadSVGPlugin(handle, _extensions);
    ret->restoreStateFromParameters();
    return ret;
}

static ReadSVGPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
