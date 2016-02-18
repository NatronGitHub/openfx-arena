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
#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif
#include <librevenge/librevenge.h>
#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>
#include <libcdr/libcdr.h>

#define kPluginName "ReadCDR"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadCDR"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0
#define kPluginEvaluation 50

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch (0 is default)"
#define kParamDpiDefault 0

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

class ReadCDRPlugin : public GenericReaderPlugin
{
public:
    ReadCDRPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadCDRPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error,int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    OFX::IntParam *dpi_;
    bool hasRSVG_;
};

ReadCDRPlugin::ReadCDRPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
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

ReadCDRPlugin::~ReadCDRPlugin()
{
}

void
ReadCDRPlugin::decode(const std::string& filename,
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
    if (!hasRSVG_) {
        setPersistentMessage(OFX::Message::eMessageError, "", "librsvg missing");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    librevenge::RVNGFileStream input(filename.c_str());
    if (!libcdr::CDRDocument::isSupported(&input))
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unsupported file format");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    librevenge::RVNGStringVector output;
    librevenge::RVNGSVGDrawingGenerator generator(output, "svg");
    if (!libcdr::CDRDocument::parse(&input, &generator))
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "SVG generation failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    if (output.empty())
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "No SVG document generated");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    std::ostringstream stream;
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
    for (unsigned k = 0; k<output.size(); ++k)
        stream << output[k].cstr();
    Magick::Blob blob(static_cast<const void *>(stream.str().c_str()),stream.str().size());

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
        image.read(blob);
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

bool ReadCDRPlugin::getFrameBounds(const std::string& filename,
                              OfxTime time,
                              OfxRectI *bounds,
                              double *par,
                              std::string* /*error*/,int *tile_width, int *tile_height)
{
    librevenge::RVNGFileStream input(filename.c_str());
    if (!libcdr::CDRDocument::isSupported(&input))
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unsupported file format");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    librevenge::RVNGStringVector output;
    librevenge::RVNGSVGDrawingGenerator generator(output, "svg");
    if (!libcdr::CDRDocument::parse(&input, &generator))
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "SVG generation failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    if (output.empty())
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "No SVG document generated");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    std::ostringstream stream;
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
    for (unsigned k = 0; k<output.size(); ++k)
        stream << output[k].cstr();
    Magick::Blob blob(static_cast<const void *>(stream.str().c_str()),stream.str().size());

    int dpi;
    dpi_->getValueAtTime(time, dpi);
    Magick::Image image;
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    if (!filename.empty())
        image.ping(blob);
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

void ReadCDRPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    assert(premult && components);

    librevenge::RVNGFileStream input(newFile.c_str());
    if (!libcdr::CDRDocument::isSupported(&input))
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unsupported file format");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    librevenge::RVNGStringVector output;
    librevenge::RVNGSVGDrawingGenerator generator(output, "svg");
    if (!libcdr::CDRDocument::parse(&input, &generator))
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "SVG generation failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    if (output.empty())
    {
        setPersistentMessage(OFX::Message::eMessageError, "", "No SVG document generated");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    std::ostringstream stream;
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
    for (unsigned k = 0; k<output.size(); ++k)
        stream << output[k].cstr();
    Magick::Blob blob(static_cast<const void *>(stream.str().c_str()),stream.str().size());

    int dpi;
    dpi_->getValue(dpi);
    Magick::Image image;
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    try {
        image.ping(blob);
    }
    catch(Magick::Warning &warning) { // ignore since warns interupt render
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
    }
    if (image.columns()>0 && image.rows()>0) {
        if (setColorSpace) {
        # ifdef OFX_IO_USING_OCIO
        _ocio->setInputColorspace("sRGB");
        # endif // OFX_IO_USING_OCIO
        }
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageUnPreMultiplied;
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadCDRPluginFactory, {}, false);

void
ReadCDRPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("cdr");
}

/** @brief The basic describe function, passed a plugin descriptor */
void ReadCDRPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    std::string plugCopyright = "\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.";
    # ifdef OFX_IO_USING_OCIO
    plugCopyright.append("\n\nOpenColorIO is Copyright 2003-2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved.\n\nOpenColorIO is distributed under a BSD license.");
    # endif // OFX_IO_USING_OCIO
    desc.setPluginDescription("Read CorelDRAW(R) document format.\n\nPowered by lcms2, librevenge, libcdr and "+magickString+plugCopyright+"\n\nThis plugin is not manufactured, approved, or supported by Corel Corporation or Corel Corporation Limited.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadCDRPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
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
ImageEffect* ReadCDRPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadCDRPlugin* ret =  new ReadCDRPlugin(handle, _extensions);
    ret->restoreStateFromParameters();
    return ret;
}

static ReadCDRPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
