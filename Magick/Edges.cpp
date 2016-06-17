/*
# Copyright (c) 2015, Ole-André Rodlie <olear@dracolinux.org>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
*/

#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include <Magick++.h>
#include <iostream>
#include <stdint.h>
#include <cmath>

#define kPluginName "EdgesOFX"
#define kPluginGrouping "Extra/Filter"
#define kPluginIdentifier "net.fxarena.openfx.Edges"
#define kPluginVersionMajor 2
#define kPluginVersionMinor 0

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

#define kParamKernel "kernel"
#define kParamKernelLabel "Kernel"
#define kParamKernelHint "Convolution Kernel"
#define kParamKernelDefault 8

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1 
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

#define kParamOpenMP "openmp"
#define kParamOpenMPLabel "OpenMP"
#define kParamOpenMPHint "Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host."
#define kParamOpenMPDefault false

using namespace OFX;
static bool _hasOpenMP = false;

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
    OFX::DoubleParam *brightness_;
    OFX::DoubleParam *smoothing_;
    OFX::DoubleParam *width_;
    OFX::BooleanParam *gray_;
    OFX::BooleanParam *enableOpenMP_;
    OFX::ChoiceParam *kernel_;
};

EdgesPlugin::EdgesPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick(NULL);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    brightness_ = fetchDoubleParam(kParamBrightness);
    smoothing_ = fetchDoubleParam(kParamSmoothing);
    width_ = fetchDoubleParam(kParamWidth);
    gray_ = fetchBooleanParam(kParamGray);
    enableOpenMP_ = fetchBooleanParam(kParamOpenMP);
    kernel_ = fetchChoiceParam(kParamKernel);

    assert(brightness_ && smoothing_ && width_ && gray_ && enableOpenMP_ && kernel_);
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
    double brightness,smoothing;
    double edge;
    int kernel;
    bool gray = false;
    bool enableOpenMP = false;
    brightness_->getValueAtTime(args.time, brightness);
    smoothing_->getValueAtTime(args.time, smoothing);
    width_->getValueAtTime(args.time, edge);
    gray_->getValueAtTime(args.time, gray);
    enableOpenMP_->getValueAtTime(args.time, enableOpenMP);
    kernel_->getValueAtTime(args.time, kernel);

    // setup
    int width = srcRod.x2-srcRod.x1;
    int height = srcRod.y2-srcRod.y1;

    // OpenMP
    unsigned int threads = 1;
    if (_hasOpenMP && enableOpenMP)
        threads = OFX::MultiThread::getNumCPUs();

    Magick::ResourceLimits::thread(threads);

    // read image
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,1)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(width,height,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());

    // grayscale
    if (gray) {
        image.quantizeColorSpace(Magick::GRAYColorspace);
        image.quantize();
    }
    // blur
    if (smoothing>0) {
        smoothing *= args.renderScale.x;
        image.blur(0,smoothing);
    }
    // edge
    std::ostringstream edgeWidth;
    edgeWidth << edge * args.renderScale.x;

    switch (kernel) {
    case 0:
        image.morphology(Magick::EdgeMorphology,Magick::BinomialKernel,edgeWidth.str());
        break;
    case 1:
        image.morphology(Magick::EdgeMorphology,Magick::LaplacianKernel,edgeWidth.str());
        break;
    case 2:
        image.morphology(Magick::EdgeMorphology,Magick::SobelKernel,edgeWidth.str());
        break;
    case 3:
        image.morphology(Magick::EdgeMorphology,Magick::FreiChenKernel,edgeWidth.str());
        break;
    case 4:
        image.morphology(Magick::EdgeMorphology,Magick::RobertsKernel,edgeWidth.str());
        break;
    case 5:
        image.morphology(Magick::EdgeMorphology,Magick::PrewittKernel,edgeWidth.str());
        break;
    case 6:
        image.morphology(Magick::EdgeMorphology,Magick::CompassKernel,edgeWidth.str());
        break;
    case 7:
        image.morphology(Magick::EdgeMorphology,Magick::KirschKernel,edgeWidth.str());
        break;
    case 8:
        image.morphology(Magick::EdgeMorphology,Magick::DiamondKernel,edgeWidth.str());
        break;
    case 9:
        image.morphology(Magick::EdgeMorphology,Magick::SquareKernel,edgeWidth.str());
        break;
    case 10:
        image.morphology(Magick::EdgeMorphology,Magick::RectangleKernel,edgeWidth.str());
        break;
    case 11:
        image.morphology(Magick::EdgeMorphology,Magick::OctagonKernel,edgeWidth.str());
        break;
    case 12:
        image.morphology(Magick::EdgeMorphology,Magick::DiskKernel,edgeWidth.str());
        break;
    case 13:
        image.morphology(Magick::EdgeMorphology,Magick::PlusKernel,edgeWidth.str());
        break;
    case 14:
        image.morphology(Magick::EdgeMorphology,Magick::CrossKernel,edgeWidth.str());
        break;
    case 15:
        image.morphology(Magick::EdgeMorphology,Magick::RingKernel,edgeWidth.str());
        break;
    case 16:
        image.morphology(Magick::EdgeMorphology,Magick::EdgesKernel,edgeWidth.str());
        break;
    case 17:
        image.morphology(Magick::EdgeMorphology,Magick::CornersKernel,edgeWidth.str());
        break;
    case 18:
        image.morphology(Magick::EdgeMorphology,Magick::DiagonalsKernel,edgeWidth.str());
        break;
    case 19:
        image.morphology(Magick::EdgeMorphology,Magick::LineEndsKernel,edgeWidth.str());
        break;
    case 20:
        image.morphology(Magick::EdgeMorphology,Magick::LineJunctionsKernel,edgeWidth.str());
        break;
    case 21:
        image.morphology(Magick::EdgeMorphology,Magick::RidgesKernel,edgeWidth.str());
        break;
    case 22:
        image.morphology(Magick::EdgeMorphology,Magick::ConvexHullKernel,edgeWidth.str());
        break;
    case 23:
        image.morphology(Magick::EdgeMorphology,Magick::ThinSEKernel,edgeWidth.str());
        break;
    case 24:
        image.morphology(Magick::EdgeMorphology,Magick::SkeletonKernel,edgeWidth.str());
        break;
    case 25:
        image.morphology(Magick::EdgeMorphology,Magick::ChebyshevKernel,edgeWidth.str());
        break;
    case 26:
        image.morphology(Magick::EdgeMorphology,Magick::ManhattanKernel,edgeWidth.str());
        break;
    case 27:
        image.morphology(Magick::EdgeMorphology,Magick::OctagonalKernel,edgeWidth.str());
        break;
    case 28:
        image.morphology(Magick::EdgeMorphology,Magick::EuclideanKernel,edgeWidth.str());
        break;
    }

    // multiply
    if (brightness>0) {
#ifdef IM7
        image.evaluate(Magick::RedChannel,Magick::MultiplyEvaluateOperator,brightness);
        image.evaluate(Magick::GreenChannel,Magick::MultiplyEvaluateOperator,brightness);
        image.evaluate(Magick::BlueChannel,Magick::MultiplyEvaluateOperator,brightness);
#else
        image.quantumOperator(Magick::RedChannel,Magick::MultiplyEvaluateOperator,brightness);
        image.quantumOperator(Magick::GreenChannel,Magick::MultiplyEvaluateOperator,brightness);
        image.quantumOperator(Magick::BlueChannel,Magick::MultiplyEvaluateOperator,brightness);
#endif
    }

    // return image
    if (dstClip_ && dstClip_->isConnected()) {
        output.composite(image, 0, 0, Magick::OverCompositeOp);
#ifdef IM7
        output.composite(image, 0, 0, Magick::CopyAlphaCompositeOp);
#else
        output.composite(image, 0, 0, Magick::CopyOpacityCompositeOp);
#endif
        output.write(0,0,args.renderWindow.x2 - args.renderWindow.x1,args.renderWindow.y2 - args.renderWindow.y1,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
    }
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
    desc.setPluginDescription("Edge extraction node.\n\nPowered by "+magickString+"\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
    desc.setHostMaskingEnabled(true);
    desc.setHostMixingEnabled(true);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void EdgesPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    std::string features = MagickCore::GetMagickFeatures();
    if (features.find("OpenMP") != std::string::npos)
        _hasOpenMP = true;

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

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWidth);
        param->setLabel(kParamWidthLabel);
        param->setHint(kParamWidthHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 50);
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
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamKernel);
        param->setLabel(kParamKernelLabel);
        param->setHint(kParamKernelHint);
        param->setDefault(kParamKernelDefault);
        param->appendOption("BinomialKernel");
        param->appendOption("LaplacianKernel");
        param->appendOption("SobelKernel");
        param->appendOption("FreiChenKernel");
        param->appendOption("RobertsKernel");
        param->appendOption("PrewittKernel");
        param->appendOption("CompassKernel");
        param->appendOption("KirschKernel");
        param->appendOption("DiamondKernel");
        param->appendOption("SquareKernel");
        param->appendOption("RectangleKernel");
        param->appendOption("OctagonKernel");
        param->appendOption("DiskKernel");
        param->appendOption("PlusKernel");
        param->appendOption("CrossKernel");
        param->appendOption("RingKernel");
        param->appendOption("EdgesKernel");
        param->appendOption("CornersKernel");
        param->appendOption("DiagonalsKernel");
        param->appendOption("LineEndsKernel");
        param->appendOption("LineJunctionsKernel");
        param->appendOption("RidgesKernel");
        param->appendOption("ConvexHullKernel");
        param->appendOption("ThinSEKernel");
        param->appendOption("SkeletonKernel");
        param->appendOption("ChebyshevKernel");
        param->appendOption("ManhattanKernel");
        param->appendOption("OctagonalKernel");
        param->appendOption("EuclideanKernel");
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamOpenMP);
        param->setLabel(kParamOpenMPLabel);
        param->setHint(kParamOpenMPHint);
        param->setDefault(kParamOpenMPDefault);
        param->setAnimates(false);
        if (!_hasOpenMP)
            param->setEnabled(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* EdgesPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new EdgesPlugin(handle);
}

static EdgesPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
