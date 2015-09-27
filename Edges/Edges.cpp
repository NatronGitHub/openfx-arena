/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "Edges.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include <Magick++.h>
#include <iostream>
#include <stdint.h>
#include <cmath>

#define kPluginName "EdgesOFX"
#define kPluginGrouping "Arena"
#define kPluginIdentifier "net.fxarena.openfx.Edges"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0
#define kPluginMagickVersion 26640

#define kParamWidth "width"
#define kParamWidthLabel "Width"
#define kParamWidthHint "Width of edges"
#define kParamWidthDefault 2

#define kParamBrightness "brightness"
#define kParamBrightnessLabel "Brightness"
#define kParamBrightnessHint "Adjust edge brightness"
#define kParamBrightnessDefault 5

#define kParamSmoothing "smoothing"
#define kParamSmoothingLabel "Smoothing"
#define kParamSmoothingHint "Adjust edge smoothing"
#define kParamSmoothingDefault 1

#define kParamGray "gray"
#define kParamGrayLabel "Grayscale"
#define kParamGrayHint "Convert to grayscale before effect"
#define kParamGrayDefault false

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 0 // TODO fix
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

using namespace OFX;

class EdgesPlugin : public OFX::ImageEffect
{
public:
    EdgesPlugin(OfxImageEffectHandle handle);
    virtual ~EdgesPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::Clip *maskClip_;
    OFX::DoubleParam *brightness_;
    OFX::DoubleParam *smoothing_;
    OFX::IntParam *width_;
    OFX::BooleanParam *gray_;
};

EdgesPlugin::EdgesPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick(NULL);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGB);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGB);
    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    assert(!maskClip_ || maskClip_->getPixelComponents() == OFX::ePixelComponentAlpha);

    brightness_ = fetchDoubleParam(kParamBrightness);
    smoothing_ = fetchDoubleParam(kParamSmoothing);
    width_ = fetchIntParam(kParamWidth);
    gray_ = fetchBooleanParam(kParamGray);

    assert(brightness_ && smoothing_ && width_ && gray_);
}

EdgesPlugin::~EdgesPlugin()
{
}

void EdgesPlugin::render(const OFX::RenderArguments &args)
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
    } else {
        OFX::throwSuiteStatusException(kOfxStatFailed);
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
    double brightness,smoothing;
    int edge;
    bool gray = false;
    brightness_->getValueAtTime(args.time, brightness);
    smoothing_->getValueAtTime(args.time, smoothing);
    width_->getValueAtTime(args.time, edge);
    gray_->getValueAtTime(args.time, gray);

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
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgb(0,0,0)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(width,height,"RGB",Magick::FloatPixel,(float*)srcImg->getPixelData());

    #ifdef DEBUG_MAGICK
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

    // grayscale
    if (gray) {
        image.quantizeColorSpace(Magick::GRAYColorspace);
        image.quantize();
    }
    // blur
    if (smoothing>0)
        image.blur(0,smoothing);
    // edge
    std::ostringstream edgeWidth;
    edgeWidth<<edge/2;
    image.morphology(Magick::EdgeMorphology,Magick::DiamondKernel,edgeWidth.str());
    // multiply
    if (brightness>0) {
        image.quantumOperator(Magick::RedChannel,Magick::MultiplyEvaluateOperator,brightness);
        image.quantumOperator(Magick::GreenChannel,Magick::MultiplyEvaluateOperator,brightness);
        image.quantumOperator(Magick::BlueChannel,Magick::MultiplyEvaluateOperator,brightness);
    }

    // return image
    if (dstClip_ && dstClip_->isConnected() && srcClip_ && srcClip_->isConnected())
        image.write(0,0,width,height,"RGB",Magick::FloatPixel,(float*)dstImg->getPixelData());
}

bool EdgesPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(EdgesPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void EdgesPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    if (magickNumber != kPluginMagickVersion)
        magickString.append("\n\nWarning! You are using an unsupported version of ImageMagick.");
    desc.setPluginDescription("Edge extraction filter node.\n\nPowered by "+magickString);

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
void EdgesPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGB);
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
    dstClip->addSupportedComponent(ePixelComponentRGB);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamWidth);
        param->setLabel(kParamWidthLabel);
        param->setHint(kParamWidthHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamWidthDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamBrightness);
        param->setLabel(kParamBrightnessLabel);
        param->setHint(kParamBrightnessHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamBrightnessDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSmoothing);
        param->setLabel(kParamSmoothingLabel);
        param->setHint(kParamSmoothingHint);
        param->setRange(0, 10);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamSmoothingDefault);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamGray);
        param->setLabel(kParamGrayLabel);
        param->setHint(kParamGrayHint);
        param->setDefault(kParamGrayDefault);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* EdgesPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new EdgesPlugin(handle);
}

void getEdgesPluginID(OFX::PluginFactoryArray &ids)
{
    static EdgesPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
