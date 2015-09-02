/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "Dragan.h"
#include "ofxsMacros.h"
#include <Magick++.h>
#include <iostream>
#include <stdint.h>
#include <cmath>

#define kPluginName "DraganOFX"
#define kPluginGrouping "Filter"
#define kPluginIdentifier "net.fxarena.openfx.Dragan"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 1
#define kPluginMagickVersion 26640

#define kParamBrightness "brightness"
#define kParamBrightnessLabel "Brightness"
#define kParamBrightnessHint "Adjust brightness factor"
#define kParamBrightnessDefault 1

#define kParamContrast "contrast"
#define kParamContrastLabel "Contrast"
#define kParamContrastHint "Adjust contrast"
#define kParamContrastDefault 0

#define kParamDarkness "darkness"
#define kParamDarknessLabel "Darkness"
#define kParamDarknessHint "Adjust shadow darkness"
#define kParamDarknessDefault 1

#define kParamSaturation "saturation"
#define kParamSaturationLabel "Saturation"
#define kParamSaturationHint "Adjust saturation"
#define kParamSaturationDefault 150

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

using namespace OFX;

class DraganPlugin : public OFX::ImageEffect
{
public:
    DraganPlugin(OfxImageEffectHandle handle);
    virtual ~DraganPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::DoubleParam *brightness_;
    OFX::DoubleParam *contrast_;
    OFX::DoubleParam *darkness_;
    OFX::IntParam *saturation_;
};

DraganPlugin::DraganPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick(NULL);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGB);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGB);

    brightness_ = fetchDoubleParam(kParamBrightness);
    contrast_ = fetchDoubleParam(kParamContrast);
    darkness_ = fetchDoubleParam(kParamDarkness);
    saturation_ = fetchIntParam(kParamSaturation);

    assert(brightness_ && contrast_ && darkness_ && saturation_);
}

DraganPlugin::~DraganPlugin()
{
}

void DraganPlugin::render(const OFX::RenderArguments &args)
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
    double brightness,contrast, darkness;
    int saturation;
    brightness_->getValueAtTime(args.time, brightness);
    contrast_->getValueAtTime(args.time, contrast);
    darkness_->getValueAtTime(args.time, darkness);
    saturation_->getValueAtTime(args.time, saturation);

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


    // WIP, still not done!!

    std::cout << "b:" << brightness << " c: " << contrast << " d: " << darkness << " s: " << saturation << std::endl;

    // enhance brightness, apply sigmoidal-contrast and saturation
    if (brightness>1) {
        image.quantumOperator(Magick::RedChannel,Magick::MultiplyEvaluateOperator,brightness);
        image.quantumOperator(Magick::GreenChannel,Magick::MultiplyEvaluateOperator,brightness);
        image.quantumOperator(Magick::BlueChannel,Magick::MultiplyEvaluateOperator,brightness);
    }
    //contrast
      if (contrast!=0)
        image.sigmoidalContrast(0,abs(contrast),50);

    image.modulate(100,saturation,100);

    // Clone, desaturate, darken and compose multiply with input
    Magick::Image image2;
    image2=image;
    image2.modulate(100,0,100);
    if (darkness>=1) {
        image2.quantumOperator(Magick::RedChannel,Magick::MultiplyEvaluateOperator,3/darkness);
        image2.quantumOperator(Magick::GreenChannel,Magick::MultiplyEvaluateOperator,3/darkness);
        image2.quantumOperator(Magick::BlueChannel,Magick::MultiplyEvaluateOperator,3/darkness);
        image2.clamp();
    }
    image2.composite(image,0,0,Magick::MultiplyCompositeOp);
    image2.clamp();

    //image=image2;

    // Clone previous result and apply high pass filter using DoG
    Magick::Image image3;
    image3=image2;
    image3.defineValue("convolve","scale","1");
    image3.artifact("convolve:bias","50%");
    image3.morphology(Magick::ConvolveMorphology,Magick::DoGKernel,"0,0,5");
    image3.clamp();

    // Clone both use overlay composite to apply high pass filter to enhanced input
    //Magick::Image image4;
    //image4=image;
    image2.composite(image3,0,0,Magick::OverlayCompositeOp);

    // Clone enhanced input and desaturate
    Magick::Image image5;
    image5=image2;
    image5.modulate(100,0,100);

    // Use hardlight composite to apply desaturated image to enhanced image
    image2.composite(image5,0,0,Magick::SoftLightCompositeOp);
    image=image2;


    // return image
    if (dstClip_ && dstClip_->isConnected() && srcClip_ && srcClip_->isConnected())
        image.write(0,0,width,height,"RGB",Magick::FloatPixel,(float*)dstImg->getPixelData());
}

bool DraganPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(DraganPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void DraganPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    if (magickNumber != kPluginMagickVersion)
        magickString.append("\n\nWarning! You are using an unsupported version of ImageMagick.");
    desc.setPluginDescription("Dragan-like effect filter node.\n\nPowered by "+magickString);

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
void DraganPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamBrightness);
        param->setLabel(kParamBrightnessLabel);
        param->setHint(kParamBrightnessHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamBrightnessDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamContrast);
        param->setLabel(kParamContrastLabel);
        param->setHint(kParamContrastHint);
        param->setRange(-100, 100);
        param->setDisplayRange(-10, 10);
        param->setDefault(kParamContrastDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamDarkness);
        param->setLabel(kParamDarknessLabel);
        param->setHint(kParamDarknessHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamDarknessDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamSaturation);
        param->setLabel(kParamSaturationLabel);
        param->setHint(kParamSaturationHint);
        param->setRange(0, 200);
        param->setDisplayRange(0, 200);
        param->setDefault(kParamSaturationDefault);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* DraganPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new DraganPlugin(handle);
}

void getDraganPluginID(OFX::PluginFactoryArray &ids)
{
    static DraganPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
