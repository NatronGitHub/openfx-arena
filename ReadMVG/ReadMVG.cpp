/*
# Copyright (c) 2015, Ole-André Rodlie <olear@dracolinux.org>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
*/

#include "ReadMVG.h"
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

#define kPluginName "ReadMVGOFX"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadMVG"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

class ReadMVGPlugin : public GenericReaderPlugin
{
public:
    ReadMVGPlugin(OfxImageEffectHandle handle);
    virtual ~ReadMVGPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void restoreState(const std::string& filename) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    int width_;
    int height_;
};

ReadMVGPlugin::ReadMVGPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
,width_(0)
,height_(0)
{
    Magick::InitializeMagick(NULL);
}

ReadMVGPlugin::~ReadMVGPlugin()
{
}

void
ReadMVGPlugin::decode(const std::string& filename,
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

bool ReadMVGPlugin::getFrameBounds(const std::string& /*filename*/,
                              OfxTime /*time*/,
                              OfxRectI *bounds,
                              double *par,
                              std::string* /*error*/,int *tile_width, int *tile_height)
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
    *tile_width = *tile_height = 0;
    return true;
}

void ReadMVGPlugin::restoreState(const std::string& filename)
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
}

void ReadMVGPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
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
    Magick::Image image;
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

mDeclareReaderPluginFactory(ReadMVGPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void ReadMVGPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"mvg", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(50);
    #endif

    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    std::string plugCopyright = "\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.";
    # ifdef OFX_IO_USING_OCIO
    plugCopyright.append("\n\nOpenColorIO is Copyright 2003-2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved.\n\nOpenColorIO is distributed under a BSD license.");
    # endif // OFX_IO_USING_OCIO
    desc.setPluginDescription("Read MVG (Magick Vector Graphics) image format.\n\nPowered by "+magickString+plugCopyright);
    
#pragma message WARN("You need to get rid of the width_ and height_ member which are not thread safe at all and very dangerous!")
    desc.setRenderThreadSafety(OFX::eRenderInstanceSafe);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadMVGPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadMVGPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadMVGPlugin* ret =  new ReadMVGPlugin(handle);
    ret->restoreStateFromParameters();
    return ret;
}

void getReadMVGPluginID(OFX::PluginFactoryArray &ids)
{
    static ReadMVGPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
