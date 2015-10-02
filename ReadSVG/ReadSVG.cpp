/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "ReadSVG.h"
#include <iostream>
#include <stdint.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadSVGOFX"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadSVG"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 4
#define kPluginMagickVersion 26640

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch (0 is default)"
#define kParamDpiDefault 0

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

class ReadSVGPlugin : public GenericReaderPlugin
{
public:
    ReadSVGPlugin(OfxImageEffectHandle handle);
    virtual ~ReadSVGPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    virtual void decode(const std::string& filename, OfxTime time, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error) OVERRIDE FINAL;
    virtual void restoreState(const std::string& filename) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    OFX::IntParam *dpi_;
    bool hasRSVG_;
    int width_;
    int height_;
};

ReadSVGPlugin::ReadSVGPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
,dpi_(0)
,hasRSVG_(false)
,width_(0)
,height_(0)
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
                      const OfxRectI& /*renderWindow*/,
                      float *pixelData,
                      const OfxRectI& bounds,
                      OFX::PixelComponentEnum /*pixelComponents*/,
                      int /*pixelComponentCount*/,
                      int /*rowBytes*/)
{
    #ifdef DEBUG
    std::cout << "decode ..." << std::endl;
    #endif

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

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
    }
    image.backgroundColor("none"); // must be set to avoid bg
    if (!filename.empty())
        image.read(filename);
    if (image.columns()>0 && image.rows()>0) {
        Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
        container.composite(image,0,0,Magick::OverCompositeOp);
        container.flip();
        container.write(0,0,bounds.x2,bounds.y2,"RGBA",Magick::FloatPixel,pixelData);
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

bool ReadSVGPlugin::getFrameBounds(const std::string& /*filename*/,
                              OfxTime /*time*/,
                              OfxRectI *bounds,
                              double *par,
                              std::string* /*error*/)
{
    #ifdef DEBUG
    std::cout << "getFrameBounds ..." << std::endl;
    #endif
    if (width_>0 && height_>0) {
        bounds->x1 = 0;
        bounds->x2 = width_;
        bounds->y1 = 0;
        bounds->y2 = height_;
        *par = 1.0;
    }
    return true;
}

void ReadSVGPlugin::restoreState(const std::string& filename)
{
    #ifdef DEBUG
    std::cout << "restoreState ..." << std::endl;
    #endif

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    Magick::Image image;
    int dpi;
    dpi_->getValue(dpi);
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    try {
        if (!filename.empty())
            image.read(filename);
    }
    catch(Magick::Warning &warning) { // ignore since warns interupt render
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
    }
    if (image.columns()>0 && image.rows()>0) {
        width_ = image.columns();
        height_ = image.rows();
    }
    /*else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }*/
}

void ReadSVGPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    #ifdef DEBUG
    std::cout << "changedParam ..." << std::endl;
    #endif

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    if (paramName == kParamDpi) {
        int dpi;
        std::string imageFile;
        _fileParam->getValue(imageFile);
        dpi_->getValue(dpi);
        Magick::Image image;
        image.resolutionUnits(Magick::PixelsPerInchResolution);
        image.density(Magick::Geometry(dpi,dpi));
        try {
            if (!imageFile.empty())
                image.read(imageFile);
        }
        catch(Magick::Warning &warning) { // ignore since warns interupt render
            #ifdef DEBUG
            std::cout << warning.what() << std::endl;
            #endif
        }
        if (image.columns()>0 && image.rows()>0) {
            width_ = image.columns();
            height_ = image.rows();
        }
    }
    else {
        GenericReaderPlugin::changedParam(args,paramName);
    }
}

void ReadSVGPlugin::onInputFileChanged(const std::string& newFile,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    #ifdef DEBUG
    std::cout << "onInputFileChanged ..." << std::endl;
    #endif

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    assert(premult && components);
    int dpi;
    dpi_->getValue(dpi);
    Magick::Image image;
    #ifdef DEBUG_MAGICK
    image.debug(true);
    #endif
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    try {
        image.read(newFile);
    }
    catch(Magick::Warning &warning) { // ignore since warns interupt render
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
    }
    if (image.columns()>0 && image.rows()>0) {
        width_ = image.columns();
        height_ = image.rows();
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
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageOpaque;
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadSVGPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void ReadSVGPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"svg","svgz", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(50);
    #endif

    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    if (magickNumber != kPluginMagickVersion)
        magickString.append("\n\nWarning! You are using an unsupported version of ImageMagick.");
    std::string delegates = MagickCore::GetMagickDelegates();
    desc.setPluginDescription("Read SVG image format.\n\nPowered by "+magickString+"\n\nFeatures: "+delegates+"\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadSVGPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamDpi);
        param->setLabel(kParamDpiLabel);
        param->setHint(kParamDpiHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamDpiDefault);
        param->setAnimates(false);
        page->addChild(*param);
    }
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadSVGPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadSVGPlugin* ret =  new ReadSVGPlugin(handle);
    ret->restoreStateFromParameters();
    return ret;
}

void getReadSVGPluginID(OFX::PluginFactoryArray &ids)
{
    static ReadSVGPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
