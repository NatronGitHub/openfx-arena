/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "Glow.h"
#include "ofxsMacros.h"
#include <Magick++.h>

#define kPluginName "GlowOFX"
#define kPluginGrouping "Filter"
#define kPluginIdentifier "net.fxarena.openfx.Glow"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 1
#define kPluginMagickVersion 26640

#define kParamAmount "amount"
#define kParamAmountLabel "Amount"
#define kParamAmountHint "Glow amount"
#define kParamAmountDefault 1.5

#define kParamSoft "softening"
#define kParamSoftLabel "Softening"
#define kParamSoftHint "Adjust softening"
#define kParamSoftDefault 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

using namespace OFX;

class GlowPlugin : public OFX::ImageEffect
{
public:
    GlowPlugin(OfxImageEffectHandle handle);
    virtual ~GlowPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::DoubleParam *amount_;
    OFX::DoubleParam *soft_;
};

GlowPlugin::GlowPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick(NULL);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGB);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGB);

    amount_ = fetchDoubleParam(kParamAmount);
    soft_ = fetchDoubleParam(kParamSoft);

    assert(amount_ && soft_);
}

GlowPlugin::~GlowPlugin()
{
}

void GlowPlugin::render(const OFX::RenderArguments &args)
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
    if (dstComponents != OFX::ePixelComponentRGB || (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
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
    double amount,soft;
    amount_->getValueAtTime(args.time, amount);
    soft_->getValueAtTime(args.time, soft);

    // setup
    int width = srcRod.x2-srcRod.x1;
    int height = srcRod.y2-srcRod.y1;

    // read image
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgb(0,0,0)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(width,height,"RGB",Magick::FloatPixel,(float*)srcImg->getPixelData());

    #ifdef DEBUG
    image.debug(true);
    #endif

    // glow
    if (amount<1)
        amount=1;
    if (soft==0) {
        image.quantumOperator(Magick::RedChannel,Magick::MultiplyEvaluateOperator,amount);
        image.quantumOperator(Magick::GreenChannel,Magick::MultiplyEvaluateOperator,amount);
        image.quantumOperator(Magick::BlueChannel,Magick::MultiplyEvaluateOperator,amount);
    }
    else if (soft>0&&amount>1){
        Magick::Image image2;
        image2=image;
        double multiply = amount-0.8;
        image2.quantumOperator(Magick::RedChannel,Magick::MultiplyEvaluateOperator,multiply);
        image2.quantumOperator(Magick::GreenChannel,Magick::MultiplyEvaluateOperator,multiply);
        image2.quantumOperator(Magick::BlueChannel,Magick::MultiplyEvaluateOperator,multiply);
        double blur = soft/3;
        image2.blur(0,blur);
        image.composite(image2,0,0,Magick::PlusCompositeOp);
    }

    // return image
    if (dstClip_ && dstClip_->isConnected() && srcClip_ && srcClip_->isConnected())
        image.write(0,0,width,height,"RGB",Magick::FloatPixel,(float*)dstImg->getPixelData());
}

bool GlowPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(GlowPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void GlowPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    if (magickNumber != kPluginMagickVersion)
        magickString.append("\n\nWarning! You are using an unsupported version of ImageMagick.");
    desc.setPluginDescription("Glow filter node.\n\nPowered by "+magickString);

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
void GlowPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGB);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamAmount);
        param->setLabel(kParamAmountLabel);
        param->setHint(kParamAmountHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamAmountDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSoft);
        param->setLabel(kParamSoftLabel);
        param->setHint(kParamSoftHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamSoftDefault);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* GlowPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new GlowPlugin(handle);
}

void getGlowPluginID(OFX::PluginFactoryArray &ids)
{
    static GlowPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
