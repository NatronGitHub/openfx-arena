/*

openfx-arena - https://github.com/olear/openfx-arena

Copyright (c) 2015, Ole-André Rodlie <olear@fxarena.net>
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Neither the name of FxArena DA nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ReadSVG.h"
#include <iostream>
#include <stdint.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadSVG"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadSVG"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 2

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch"
#define kParamDpiDefault 72

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

#define DEBUG_MAGICK false

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
    if (!hasRSVG_)
        setPersistentMessage(OFX::Message::eMessageError, "", "librsvg missing");
    int dpi = 0;
    dpi_->getValueAtTime(time, dpi);
    Magick::Image image;
    #ifdef DEBUG
    image.debug(DEBUG_MAGICK);
    #endif
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    image.read(filename);
    if (image.columns()>0 && image.rows()>0) {
        Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
        #ifdef DEBUG
        container.debug(DEBUG_MAGICK);
        #endif
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
    Magick::Image image;
    #ifdef DEBUG
    image.debug(DEBUG_MAGICK);
    #endif
    int dpi;
    dpi_->getValue(dpi);
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    image.read(filename);
    if (image.columns()>0 && image.rows()>0) {
        width_ = image.columns();
        height_ = image.rows();
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

void ReadSVGPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    #ifdef DEBUG
    std::cout << "changedParam ..." << std::endl;
    #endif
    if (paramName == kParamDpi) {
        int dpi;
        std::string imageFile;
        _fileParam->getValue(imageFile);
        dpi_->getValue(dpi);
        #ifdef DEBUG
        std::cout << "changed dpi to " << dpi << std::endl;
        #endif
        Magick::Image image;
        #ifdef DEBUG
        image.debug(DEBUG_MAGICK);
        #endif
        image.resolutionUnits(Magick::PixelsPerInchResolution);
        image.density(Magick::Geometry(dpi,dpi));
        image.read(imageFile);
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
    assert(premult && components);
    Magick::Image image;
    #ifdef DEBUG
    image.debug(DEBUG_MAGICK);
    #endif
    int dpi;
    dpi_->getValue(dpi);
    image.resolutionUnits(Magick::PixelsPerInchResolution);
    image.density(Magick::Geometry(dpi,dpi));
    image.read(newFile);
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

    std::string magickV = MagickCore::GetMagickVersion(NULL);
    std::string delegates = MagickCore::GetMagickDelegates();
    desc.setPluginDescription("Read SVG image format.\n\nWritten by Ole-André Rodlie <olear@fxarena.net>\n\nPowered by "+magickV+"\n\nFeatures: "+delegates);
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
