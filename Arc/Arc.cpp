/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "Arc.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include <Magick++.h>
#include <iostream>
#include <stdint.h>
#include <cmath>

#define kPluginName "ArcOFX"
#define kPluginGrouping "Extra/Distort"
#define kPluginIdentifier "net.fxarena.openfx.Arc"
#define kPluginVersionMajor 3
#define kPluginVersionMinor 1

#define kParamVPixel "pixel"
#define kParamVPixelLabel "Virtual Pixel"
#define kParamVPixelHint "Virtual Pixel Method"
#define kParamVPixelDefault 12

#define kParamArcAngle "angle"
#define kParamArcAngleLabel "Angle"
#define kParamArcAngleHint "Arc angle"
#define kParamArcAngleDefault 60

#define kParamArcRotate "rotate"
#define kParamArcRotateLabel "Rotate"
#define kParamArcRotateHint "Arc rotate"
#define kParamArcRotateDefault 0

#define kParamArcTopRadius "top"
#define kParamArcTopRadiusLabel "Top radius"
#define kParamArcTopRadiusHint "Arc top radius"
#define kParamArcTopRadiusDefault 0

#define kParamArcBottomRadius "bottom"
#define kParamArcBottomRadiusLabel "Bottom radius"
#define kParamArcBottomRadiusHint "Arc bottom radius"
#define kParamArcBottomRadiusDefault 0

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

class ArcPlugin : public OFX::ImageEffect
{
public:
    ArcPlugin(OfxImageEffectHandle handle);
    virtual ~ArcPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::ChoiceParam *vpixel_;
    OFX::DoubleParam *arcAngle_;
    OFX::DoubleParam *arcRotate_;
    OFX::DoubleParam *arcTopRadius_;
    OFX::DoubleParam *arcBottomRadius_;
    OFX::BooleanParam *matte_;
    OFX::BooleanParam *scale_;
};

ArcPlugin::ArcPlugin(OfxImageEffectHandle handle)
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
    arcAngle_ = fetchDoubleParam(kParamArcAngle);
    arcRotate_ = fetchDoubleParam(kParamArcRotate);
    arcTopRadius_ = fetchDoubleParam(kParamArcTopRadius);
    arcBottomRadius_ = fetchDoubleParam(kParamArcBottomRadius);
    matte_ = fetchBooleanParam(kParamMatte);
    scale_ = fetchBooleanParam(kParamScale);

    assert(vpixel_ && arcAngle_ && arcRotate_ && arcTopRadius_&& arcBottomRadius_ && matte_ && scale_);
}

ArcPlugin::~ArcPlugin()
{
}

void ArcPlugin::render(const OFX::RenderArguments &args)
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
    int vpixel;
    double arcAngle,arcRotate,arcTopRadius,arcBottomRadius;
    bool matte = false;
    bool scale = false;
    vpixel_->getValueAtTime(args.time, vpixel);
    arcAngle_->getValueAtTime(args.time, arcAngle);
    arcRotate_->getValueAtTime(args.time, arcRotate);
    arcTopRadius_->getValueAtTime(args.time, arcTopRadius);
    arcBottomRadius_->getValueAtTime(args.time, arcBottomRadius);
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

    #ifdef DEBUG_MAGICK
    image.debug(true);
    #endif

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

    // distort method
    double distortArgs[4];
    int distortOpts = 0;
    if (arcAngle!=0) {
        distortArgs[distortOpts] = arcAngle;
        distortOpts++;
    }
    if (arcRotate!=0) {
        distortArgs[distortOpts] = arcRotate;
        distortOpts++;
    }
    else {
        distortArgs[distortOpts] = 0;
        distortOpts++;
    }
    if (arcTopRadius!=0) {
        distortArgs[distortOpts] = std::floor(arcTopRadius * args.renderScale.x + 0.5);
        distortOpts++;
    }
    if (arcBottomRadius!=0 && arcTopRadius!=0) {
        distortArgs[distortOpts] = std::floor(arcBottomRadius * args.renderScale.x + 0.5);
        distortOpts++;
    }
    image.backgroundColor(Magick::Color("rgba(0,0,0,0)"));
    if (matte) {
        image.matte(false);
        image.matte(true);
    }
    image.distort(Magick::ArcDistortion, distortOpts, distortArgs, Magick::MagickTrue);

    if (scale) {
        std::ostringstream scaleW;
        scaleW << width << "x";
        std::ostringstream scaleH;
        scaleH << "x" << height;
        std::size_t columns = width;
        std::size_t rows = height;
        if (image.columns()>columns)
            image.scale(scaleW.str());
        if (image.rows()>rows)
            image.scale(scaleH.str());
    }

    // return image
    if (!scale) // may be a diff of 1px, so to be safe just extend to dstBounds
        image.extent(Magick::Geometry(dstBounds.x2-dstBounds.x1,dstBounds.y2-dstBounds.y1),Magick::CenterGravity);
    else
        image.extent(Magick::Geometry(width,height),Magick::CenterGravity);
    image.flip();
    if (dstClip_ && dstClip_->isConnected()) {
        width = dstBounds.x2-dstBounds.x1;
        height = dstBounds.y2-dstBounds.y1;
        int widthstep = width*4;
        int imageSize = width*height*4;
        float* imageBlock;
        imageBlock = new float[imageSize];
        image.write(0,0,width,height,"RGBA",Magick::FloatPixel,imageBlock);
        for(int y = args.renderWindow.y1; y < (args.renderWindow.y1 + height); y++) {
            OfxRGBAColourF *dstPix = (OfxRGBAColourF *)dstImg->getPixelAddress(args.renderWindow.x1, y);
            float *srcPix = (float*)(imageBlock + y * widthstep + args.renderWindow.x1);
            for(int x = args.renderWindow.x1; x < (args.renderWindow.x1 + width); x++) {
                dstPix->r = srcPix[0]*srcPix[3];
                dstPix->g = srcPix[1]*srcPix[3];
                dstPix->b = srcPix[2]*srcPix[3];
                dstPix->a = srcPix[3];
                dstPix++;
                srcPix+=4;
            }
        }
        free(imageBlock);
    }
}

bool ArcPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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
            double arcAngle,arcRotate,arcTopRadius,arcBottomRadius;
            arcAngle_->getValueAtTime(args.time, arcAngle);
            arcRotate_->getValueAtTime(args.time, arcRotate);
            arcTopRadius_->getValueAtTime(args.time, arcTopRadius);
            arcBottomRadius_->getValueAtTime(args.time, arcBottomRadius);
            const OfxRectD& srcRod = srcClip_->getRegionOfDefinition(args.time);
            int width = srcRod.x2-srcRod.x1;
            int height = srcRod.y2-srcRod.y1;
            Magick::Image image;
            image.size(Magick::Geometry(width,height));
            double distortArgs[4];
            int distortOpts = 0;
            if (arcAngle!=0) {
                distortArgs[distortOpts] = arcAngle;
                distortOpts++;
            }
            if (arcRotate!=0) {
                distortArgs[distortOpts] = arcRotate;
                distortOpts++;
            }
            else {
                distortArgs[distortOpts] = 0;
                distortOpts++;
            }
            if (arcTopRadius!=0) {
                distortArgs[distortOpts] = std::floor(arcTopRadius * args.renderScale.x + 0.5);
                distortOpts++;
            }
            if (arcBottomRadius!=0 && arcTopRadius!=0) {
                distortArgs[distortOpts] = std::floor(arcBottomRadius * args.renderScale.x + 0.5);
                distortOpts++;
            }
            image.distort(Magick::ArcDistortion, distortOpts, distortArgs, Magick::MagickTrue);
            int w = (int)image.columns();
            int h = (int)image.rows();
            rod = srcRod;
            rod.x1 = srcRod.x1;
            rod.x2 = w;
            rod.y1 = srcRod.y1;
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

mDeclarePluginFactory(ArcPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void ArcPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    desc.setPluginDescription("Arc Distort transform node.\n\nPowered by "+magickString+"\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.");

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
void ArcPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcAngle);
        param->setLabel(kParamArcAngleLabel);
        param->setHint(kParamArcAngleHint);
        param->setRange(1, 360);
        param->setDisplayRange(1, 360);
        param->setDefault(kParamArcAngleDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcRotate);
        param->setLabel(kParamArcRotateLabel);
        param->setHint(kParamArcRotateHint);
        param->setRange(0, 360);
        param->setDisplayRange(0, 360);
        param->setDefault(kParamArcRotateDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcTopRadius);
        param->setLabel(kParamArcTopRadiusLabel);
        param->setHint(kParamArcTopRadiusHint);
        param->setRange(0, 360);
        param->setDisplayRange(0, 360);
        param->setDefault(kParamArcTopRadiusDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcBottomRadius);
        param->setLabel(kParamArcBottomRadiusLabel);
        param->setHint(kParamArcBottomRadiusHint);
        param->setRange(0, 360);
        param->setDisplayRange(0, 360);
        param->setDefault(kParamArcBottomRadiusDefault);
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
ImageEffect* ArcPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new ArcPlugin(handle);
}

void getArcPluginID(OFX::PluginFactoryArray &ids)
{
    static ArcPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
