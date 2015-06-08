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

#include "Polar.h"

#include "ofxsMacros.h"
#include <Magick++.h>

#include <iostream>
#include <stdint.h>

#define kPluginName "Polar"
#define kPluginGrouping "Filter"
#define kPluginDescription "Distort image using Polar technique"

#define kPluginIdentifier "net.fxarena.openfx.Polar"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kParamVPixel "virtualPixelMethod"
#define kParamVPixelLabel "Virtual Pixel"
#define kParamVPixelHint "Virtual Pixel Method"
#define kParamVPixelDefault 12

#define kParamPolarRotate "polarRotate"
#define kParamPolarRotateLabel "Rotate"
#define kParamPolarRotateHint "Polar rotate"
#define kParamPolarRotateDefault 0

#define kParamDePolar "dePolar"
#define kParamDePolarLabel "DePolar"
#define kParamDePolarHint "Distort image using DePolar technique"
#define kParamDePolarDefault false

#define kParamPolarFlip "polarFlip"
#define kParamPolarFlipLabel "Flip"
#define kParamPolarFlipHint "Polar Flip"
#define kParamPolarFlipDefault false

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

using namespace OFX;

class PolarPlugin : public OFX::ImageEffect
{
public:
    PolarPlugin(OfxImageEffectHandle handle);
    virtual ~PolarPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::ChoiceParam *vpixel_;
    OFX::DoubleParam *polarRotate_;
    OFX::BooleanParam *dePolar_;
    OFX::BooleanParam *polarFlip_;
};

PolarPlugin::PolarPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick(NULL);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    vpixel_ = fetchChoiceParam(kParamVPixel);
    polarRotate_ = fetchDoubleParam(kParamPolarRotate);
    dePolar_ = fetchBooleanParam(kParamDePolar);
    polarFlip_ = fetchBooleanParam(kParamPolarFlip);
    assert(vpixel_ && polarRotate_ && dePolar_ && polarFlip_);
}

PolarPlugin::~PolarPlugin()
{
}

void PolarPlugin::render(const OFX::RenderArguments &args)
{
    // render scale
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get src clip
    if (!srcClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(srcClip_);
    std::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        if (srcImg->getRenderScale().x != args.renderScale.x ||
            srcImg->getRenderScale().y != args.renderScale.y ||
            srcImg->getField() != args.fieldToRender) {
            setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
            OFX::throwSuiteStatusException(kOfxStatFailed);
            return;
        }
    }

    // get dest clip
    if (!dstClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);
    std::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    if (dstImg->getRenderScale().x != args.renderScale.x ||
        dstImg->getRenderScale().y != args.renderScale.y ||
        dstImg->getField() != args.fieldToRender) {
        setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get bit depth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && (dstBitDepth != srcImg->getPixelDepth()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get pixel component
    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != OFX::ePixelComponentRGBA || (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds?
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // get params
    int vpixel;
    double polarRotate;
    bool polarFlip = false;
    bool dePolar = false;
    vpixel_->getValueAtTime(args.time, vpixel);
    polarRotate_->getValueAtTime(args.time, polarRotate);
    dePolar_->getValueAtTime(args.time, dePolar);
    polarFlip_->getValueAtTime(args.time, polarFlip);

    // setup
    int width = srcRod.x2-srcRod.x1;
    int height = srcRod.y2-srcRod.y1;

    // read image
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(width,height,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());

    image.flip();

    // set virtual pixel
    switch (vpixel) {
    case 0:
        image.virtualPixelMethod(Magick::UndefinedVirtualPixelMethod);
        break;
    case 1:
        image.virtualPixelMethod(Magick::BackgroundVirtualPixelMethod);
        break;
    case 2:
        image.virtualPixelMethod(Magick::BlackVirtualPixelMethod);
        break;
    case 3:
        image.virtualPixelMethod(Magick::CheckerTileVirtualPixelMethod);
        break;
    case 4:
        image.virtualPixelMethod(Magick::DitherVirtualPixelMethod);
        break;
    case 5:
        image.virtualPixelMethod(Magick::EdgeVirtualPixelMethod);
        break;
    case 6:
        image.virtualPixelMethod(Magick::GrayVirtualPixelMethod);
        break;
    case 7:
        image.virtualPixelMethod(Magick::HorizontalTileVirtualPixelMethod);
        break;
    case 8:
        image.virtualPixelMethod(Magick::HorizontalTileEdgeVirtualPixelMethod);
        break;
    case 9:
        image.virtualPixelMethod(Magick::MirrorVirtualPixelMethod);
        break;
    case 10:
        image.virtualPixelMethod(Magick::RandomVirtualPixelMethod);
        break;
    case 11:
        image.virtualPixelMethod(Magick::TileVirtualPixelMethod);
        break;
    case 12:
        image.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
        break;
    case 13:
        image.virtualPixelMethod(Magick::VerticalTileVirtualPixelMethod);
        break;
    case 14:
        image.virtualPixelMethod(Magick::VerticalTileEdgeVirtualPixelMethod);
        break;
    case 15:
        image.virtualPixelMethod(Magick::WhiteVirtualPixelMethod);
        break;
    }

    double distortArgs[4];
    std::ostringstream scaleW;
    scaleW << width << "x";
    std::ostringstream scaleH;
    scaleH << "x" << height;
    image.backgroundColor(Magick::Color("rgba(0,0,0,0)"));
    if (polarFlip)
        image.flip();
    if (!dePolar) {
        image.distort(Magick::PolarDistortion, 0, distortArgs, Magick::MagickTrue);
        if (image.rows()>height)
            image.scale(scaleH.str());
    }
    else {
        image.distort(Magick::DePolarDistortion, 0, distortArgs, Magick::MagickFalse);
        if (image.columns()>width)
            image.scale(scaleW.str());
        if (image.rows()>height)
            image.scale(scaleH.str());
    }
    if (polarRotate!=0)
        image.rotate(polarRotate);
    image.flip();
    image.extent(Magick::Geometry(width,height),Magick::CenterGravity);

    // return image
    if (dstClip_ && dstClip_->isConnected() && srcClip_ && srcClip_->isConnected()) {
        switch (dstBitDepth) {
        case eBitDepthUByte:
            if (image.depth()>8)
                image.depth(8);
            image.write(0,0,width,height,"RGBA",Magick::CharPixel,(float*)dstImg->getPixelData());
            break;
        case eBitDepthUShort:
            if (image.depth()>16)
                image.depth(16);
            image.write(0,0,width,height,"RGBA",Magick::ShortPixel,(float*)dstImg->getPixelData());
            break;
        case eBitDepthFloat:
            image.write(0,0,width,height,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
            break;
        }
    }
}

bool PolarPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }
    if (srcClip_ && srcClip_->isConnected()) {
        rod = srcClip_->getRegionOfDefinition(args.time);
    } else {
        rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
    }
    return true;
}

mDeclarePluginFactory(PolarPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void PolarPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    //desc.addSupportedBitDepth(eBitDepthUByte);
    //desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void PolarPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages and to things in
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamPolarRotate);
        param->setLabel(kParamPolarRotateLabel);
        param->setHint(kParamPolarRotateHint);
        param->setRange(-360, 360);
        param->setDisplayRange(-360, 360);
        param->setDefault(kParamPolarRotateDefault);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamDePolar);
        param->setLabel(kParamDePolarLabel);
        param->setHint(kParamDePolarHint);
        param->setDefault(kParamDePolarDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamPolarFlip);
        param->setLabel(kParamPolarFlipLabel);
        param->setHint(kParamPolarFlipHint);
        param->setDefault(kParamPolarFlipDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamVPixel);
        param->setLabel(kParamVPixelLabel);
        param->setHint(kParamVPixelHint);
        param->appendOption("Undefined");
        param->appendOption("Background");
        param->appendOption("Black");
        param->appendOption("CheckerTile");
        param->appendOption("Dither");
        param->appendOption("Edge");
        param->appendOption("Gray");
        param->appendOption("HorizontalTile");
        param->appendOption("HorizontalTileEdge");
        param->appendOption("Mirror");
        param->appendOption("Random");
        param->appendOption("Tile");
        param->appendOption("Transparent");
        param->appendOption("VerticalTile");
        param->appendOption("VerticalTileEdge");
        param->appendOption("White");
        param->setDefault(kParamVPixelDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* PolarPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new PolarPlugin(handle);
}

void getPolarPluginID(OFX::PluginFactoryArray &ids)
{
    static PolarPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
