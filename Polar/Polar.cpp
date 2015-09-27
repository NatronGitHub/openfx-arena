/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "Polar.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include <Magick++.h>
#include <iostream>
#include <stdint.h>
#include <cmath>

#define kPluginName "PolarOFX"
#define kPluginGrouping "Arena"
#define kPluginIdentifier "net.fxarena.openfx.Polar"
#define kPluginVersionMajor 3
#define kPluginVersionMinor 0
#define kPluginMagickVersion 26640

#define kParamVPixel "pixel"
#define kParamVPixelLabel "Virtual Pixel"
#define kParamVPixelHint "Virtual Pixel Method"
#define kParamVPixelDefault 12

#define kParamPolarRotate "rotate"
#define kParamPolarRotateLabel "Rotate"
#define kParamPolarRotateHint "Polar rotate"
#define kParamPolarRotateDefault 0

#define kParamDePolar "dePolar"
#define kParamDePolarLabel "DePolar"
#define kParamDePolarHint "DePolar"
#define kParamDePolarDefault false

#define kParamPolarFlip "flip"
#define kParamPolarFlipLabel "Flip"
#define kParamPolarFlipHint "Polar Flip"
#define kParamPolarFlipDefault false

#define kParamScale "scale"
#define kParamScaleLabel "Scale"
#define kParamScaleHint "Force scale to original image size"
#define kParamScaleDefault true

#define kParamMatte "matte"
#define kParamMatteLabel "Matte"
#define kParamMatteHint "Merge Alpha before applying effect"
#define kParamMatteDefault false

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

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
    OFX::Clip *maskClip_;
    OFX::ChoiceParam *vpixel_;
    OFX::DoubleParam *polarRotate_;
    OFX::BooleanParam *polarFlip_;
    OFX::BooleanParam *dePolar_;
    OFX::BooleanParam *matte_;
    OFX::BooleanParam *scale_;
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
    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    assert(!maskClip_ || maskClip_->getPixelComponents() == OFX::ePixelComponentAlpha);

    vpixel_ = fetchChoiceParam(kParamVPixel);
    polarRotate_ = fetchDoubleParam(kParamPolarRotate);
    polarFlip_ = fetchBooleanParam(kParamPolarFlip);
    dePolar_ = fetchBooleanParam(kParamDePolar);
    matte_ = fetchBooleanParam(kParamMatte);
    scale_ = fetchBooleanParam(kParamScale);

    assert(vpixel_ && polarRotate_ && polarFlip_ && dePolar_ && matte_ && scale_);
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

    // Get mask clip
    std::auto_ptr<const OFX::Image> maskImg((getContext() != OFX::eContextFilter && maskClip_ && maskClip_->isConnected()) ? maskClip_->fetchImage(args.time) : 0);
    OfxRectI maskRod;
    if (maskImg.get())
        maskRod=maskImg->getRegionOfDefinition();

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
    bool matte = false;
    bool scale = false;
    vpixel_->getValueAtTime(args.time, vpixel);
    polarRotate_->getValueAtTime(args.time, polarRotate);
    polarFlip_->getValueAtTime(args.time, polarFlip);
    dePolar_->getValueAtTime(args.time, dePolar);
    matte_->getValueAtTime(args.time, matte);
    scale_->getValueAtTime(args.time, scale);

    // setup
    int width = srcRod.x2-srcRod.x1;
    int height = srcRod.y2-srcRod.y1;

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    // read image
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(width,height,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());

    #ifdef DEBUG
    image.debug(true);
    #endif

    // apply mask
    if (maskImg.get()) {
        int maskWidth = maskRod.x2-maskRod.x1;
        int maskHeight = maskRod.y2-maskRod.y1;
        if (maskWidth>0 && maskHeight>0) {
            Magick::Image mask(maskWidth,maskHeight,"A",Magick::FloatPixel,(float*)maskImg->getPixelData());
            int offsetX = 0;
            int offsetY = 0;
            if (maskRod.x1!=0)
                offsetX = maskRod.x1;
            if (maskRod.y1!=0)
                offsetY = maskRod.y1;
            image.composite(mask,offsetX,offsetY,Magick::CopyOpacityCompositeOp);
            Magick::Image container(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
            container.composite(image,0,0,Magick::OverCompositeOp);
            image=container;
        }
    }

    // flip
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

    // distort
    double distortArgs[0];
    image.backgroundColor(Magick::Color("rgba(0,0,0,0)"));
    if (matte) {
        image.matte(false);
        image.matte(true);
    }
    if (polarFlip)
        image.flip();
    if (dePolar)
        image.distort(Magick::DePolarDistortion, 0, distortArgs, Magick::MagickFalse);
    else
        image.distort(Magick::PolarDistortion, 0, distortArgs, Magick::MagickTrue);
    if (polarRotate!=0)
        image.rotate(polarRotate);

    if (scale) {
        std::ostringstream scaleW;
        scaleW << width << "x";
        std::ostringstream scaleH;
        scaleH << "x" << height;
        std::size_t rows = height;
        if (image.rows()>rows)
            image.scale(scaleH.str());
    }

    // return image
    if (scale)
        image.extent(Magick::Geometry(width,height),Magick::CenterGravity);
    else
        image.extent(Magick::Geometry(dstBounds.x2,dstBounds.y2),Magick::CenterGravity);
    image.flip();
    if (dstClip_ && dstClip_->isConnected() && srcClip_ && srcClip_->isConnected())
        image.write(0,0,dstBounds.x2,dstBounds.y2,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
}

bool PolarPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    if (srcClip_ && srcClip_->isConnected()) {
        bool scale = false;
        scale_->getValueAtTime(args.time, scale);
        if (!scale) {
            double polarRotate;
            bool dePolar = false;
            polarRotate_->getValueAtTime(args.time, polarRotate);
            dePolar_->getValueAtTime(args.time, dePolar);
            const OfxRectD& srcRod = srcClip_->getRegionOfDefinition(args.time);
            int width = srcRod.x2-srcRod.x1;
            int height = srcRod.y2-srcRod.y1;
            double distortArgs[0];
            Magick::Image image;
            image.size(Magick::Geometry(width,height));
            if (dePolar)
                image.distort(Magick::DePolarDistortion, 0, distortArgs, Magick::MagickFalse);
            else
                image.distort(Magick::PolarDistortion, 0, distortArgs, Magick::MagickTrue);
            if (polarRotate!=0)
                image.rotate(polarRotate);
            int w = (int)image.columns();
            int h = (int)image.rows();
            rod = srcRod;
            rod.x1 = 0;
            rod.x2 = w;
            rod.y1 = 0;
            rod.y2 = h;
        }
        else
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
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    if (magickNumber != kPluginMagickVersion)
        magickString.append("\n\nWarning! You are using an unsupported version of ImageMagick.");
    desc.setPluginDescription("Polar Distort transform node.\n\nPowered by "+magickString);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
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

    // create optional mask clip
    ClipDescriptor *maskClip = desc.defineClip("Mask");
    maskClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    maskClip->setTemporalClipAccess(false);
    maskClip->setOptional(true);
    maskClip->setSupportsTiles(kSupportsTiles);
    maskClip->setIsMask(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
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
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamScale);
        param->setLabel(kParamScaleLabel);
        param->setHint(kParamScaleHint);
        param->setDefault(kParamScaleDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamMatte);
        param->setLabel(kParamMatteLabel);
        param->setHint(kParamMatteHint);
        param->setDefault(kParamMatteDefault);
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
