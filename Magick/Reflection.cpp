/*
 * openfx-arena <https://github.com/rodlie/openfx-arena>,
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-arena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
*/

#include "MagickCommon.h"

#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include <stdint.h>
#include <cmath>
#include <iostream>

#define kPluginName "ReflectionOFX"
#define kPluginGrouping "Extra/Transform"
#define kPluginIdentifier "net.fxarena.openfx.Reflection"
#define kPluginVersionMajor 3
#define kPluginVersionMinor 2

#define kParamSpace "spacing"
#define kParamSpaceLabel "Reflection spacing"
#define kParamSpaceHint "Space between image and reflection"
#define kParamSpaceDefault 0

#define kParamOffset "offset"
#define kParamOffsetLabel "Reflection offset"
#define kParamOffsetHint "Reflection offset"
#define kParamOffsetDefault 0

#define kParamMatte "matte"
#define kParamMatteLabel "Matte"
#define kParamMatteHint "Merge Alpha before applying effect"
#define kParamMatteDefault false

#define kParamReflection "reflection"
#define kParamReflectionLabel "Reflection"
#define kParamReflectionHint "Apply reflection"
#define kParamReflectionDefault true

#define kParamMirror "mirror"
#define kParamMirrorLabel "Mirror"
#define kParamMirrorHint "Select mirror type"
#define kParamMirrorDefault 0

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

class ReflectionPlugin : public OFX::ImageEffect
{
public:
    ReflectionPlugin(OfxImageEffectHandle handle);
    virtual ~ReflectionPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::IntParam *spacing_;
    OFX::IntParam *offset_;
    OFX::BooleanParam *matte_;
    OFX::BooleanParam *reflection_;
    OFX::ChoiceParam *mirror_;
    OFX::BooleanParam *enableOpenMP_;
};

ReflectionPlugin::ReflectionPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(NULL)
, srcClip_(NULL)
, spacing_(NULL)
, offset_(NULL)
, matte_(NULL)
, reflection_(NULL)
, mirror_(NULL)
, enableOpenMP_(NULL)
{
    Magick::InitializeMagick(NULL);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    spacing_ = fetchIntParam(kParamSpace);
    offset_ = fetchIntParam(kParamOffset);
    matte_ = fetchBooleanParam(kParamMatte);
    reflection_ = fetchBooleanParam(kParamReflection);
    mirror_ = fetchChoiceParam(kParamMirror);
    enableOpenMP_ = fetchBooleanParam(kParamOpenMP);

    assert(spacing_ && offset_ && matte_ && mirror_ && reflection_ && enableOpenMP_);
}

ReflectionPlugin::~ReflectionPlugin()
{
}

void ReflectionPlugin::render(const OFX::RenderArguments &args)
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
    OFX::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        checkBadRenderScaleOrField(srcImg, args);
    } else {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // get dest clip
    if (!dstClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);
    OFX::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    checkBadRenderScaleOrField(dstImg, args);

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
    int spacing = 0;
    int offset = 0;
    int mirror = 0;
    bool matte = false;
    bool reflection = false;
    bool enableOpenMP = false;
    mirror_->getValueAtTime(args.time, mirror);
    spacing_->getValueAtTime(args.time, spacing);
    offset_->getValueAtTime(args.time, offset);
    matte_->getValueAtTime(args.time, matte);
    reflection_->getValueAtTime(args.time, reflection);
    enableOpenMP_->getValueAtTime(args.time, enableOpenMP);

    // setup
    int srcWidth = srcRod.x2-srcRod.x1;
    int srcHeight = srcRod.y2-srcRod.y1;
    int mirrorWidth = srcWidth/2;
    int mirrorHeight = srcHeight/2;
    Magick::Image image;
    Magick::Image image0;
    Magick::Image image1;
    Magick::Image image2;
    Magick::Image image3;
    Magick::Image image4;

    // OpenMP
#ifndef LEGACYIM
    unsigned int threads = 1;
    if (_hasOpenMP && enableOpenMP)
        threads = OFX::MultiThread::getNumCPUs();

    Magick::ResourceLimits::thread(threads);
#endif

    // read source image
    Magick::Image container(Magick::Geometry(srcWidth,srcHeight),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(srcWidth,srcHeight),Magick::Color("rgba(0,0,0,1)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(srcWidth,srcHeight,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());

    if (matte) {
#if MagickLibVersion >= 0x700
        image.alpha(false);
        image.alpha(true);
#else
        image.matte(false);
        image.matte(true);
#endif
    }

    // proc mirror
    Magick::Image mirrorContainer(Magick::Geometry(srcWidth,srcHeight),Magick::Color("rgba(0,0,0,0)"));
    switch(mirror) {
    case 1: // North
        image1 = image;
        image1.flip();
        image.crop(Magick::Geometry(srcWidth,mirrorHeight,0,mirrorHeight));
        image1.crop(Magick::Geometry(srcWidth,mirrorHeight,0,0));
        mirrorContainer.composite(image,0,mirrorHeight,Magick::OverCompositeOp);
        mirrorContainer.composite(image1,0,0,Magick::OverCompositeOp);
        break;
    case 2: // South
        image1 = image;
        image.flip();
        image.crop(Magick::Geometry(srcWidth,mirrorHeight,0,mirrorHeight));
        image1.crop(Magick::Geometry(srcWidth,mirrorHeight,0,0));
        mirrorContainer.composite(image,0,mirrorHeight,Magick::OverCompositeOp);
        mirrorContainer.composite(image1,0,0,Magick::OverCompositeOp);
        break;
    case 3: // East
        image1 = image;
        image1.flop();
        image.crop(Magick::Geometry(mirrorWidth,srcHeight,mirrorWidth,0));
        image1.crop(Magick::Geometry(mirrorWidth,srcHeight,0,0));
        mirrorContainer.composite(image,mirrorWidth,0,Magick::OverCompositeOp);
        mirrorContainer.composite(image1,0,0,Magick::OverCompositeOp);
        break;
    case 4: // West
        image1 = image;
        image.flop();
        image.crop(Magick::Geometry(mirrorWidth,srcHeight,mirrorWidth,0));
        image1.crop(Magick::Geometry(mirrorWidth,srcHeight,0,0));
        mirrorContainer.composite(image,mirrorWidth,0,Magick::OverCompositeOp);
        mirrorContainer.composite(image1,0,0,Magick::OverCompositeOp);
        break;
    case 5: // NorthWest
        image1 = image;
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,0,mirrorHeight));
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 6: // NorthEast
        image1 = image;
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,mirrorWidth,mirrorHeight));
        image1.flop();
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 7: // SouthWest
        image1 = image;
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,0,0));
        image1.flip();
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 8: // SouthEast
        image1 = image;
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,mirrorWidth,0));
        image1.flop();
        image1.flip();
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 9: // flip
        image.flip();
        break;
    case 10: // flop
        image.flop();
        break;
    case 11: //flip+flop
        image.flip();
        image.flop();
        break;
    default:
        // none
        break;
    }
    if (mirror>4&&mirror<9) {
        mirrorContainer.composite(image1,0,mirrorHeight,Magick::OverCompositeOp);
        mirrorContainer.composite(image2,mirrorWidth,mirrorHeight,Magick::OverCompositeOp);
        mirrorContainer.composite(image3,mirrorWidth,0,Magick::OverCompositeOp);
        mirrorContainer.composite(image4,0,0,Magick::OverCompositeOp);
    }
    if (mirror>0&&mirror<9)
        image=mirrorContainer;
    if (reflection) {
        image0 = image;
        image0.flip();
        spacing = std::floor(spacing * args.renderScale.x + 0.5);
        offset = std::floor(offset * args.renderScale.x + 0.5);
        if (offset>=mirrorHeight)
            offset=mirrorHeight-1;
        if (offset<0) {
            if ((offset-offset*2)>=mirrorHeight)
                offset = (mirrorHeight*-1)+1;
            image0.crop(Magick::Geometry(srcWidth,mirrorHeight-offset,0,offset-offset));
            image.crop(Magick::Geometry(srcWidth,mirrorHeight+offset,0,mirrorHeight+offset));
        }
        else {
            image0.crop(Magick::Geometry(srcWidth,mirrorHeight-offset,0,offset+offset));
            image.crop(Magick::Geometry(srcWidth,mirrorHeight+offset,0,mirrorHeight-offset));
        }
        /*if (maskImg.get()) {
            int maskWidth = maskRod.x2-maskRod.x1;
            int maskHeight = maskRod.y2-maskRod.y1;
            if (maskWidth>0&&maskHeight>0) {
                Magick::Image mask(maskWidth,maskHeight,"A",Magick::FloatPixel,(float*)maskImg->getPixelData());
                int offsetX = 0;
                int offsetY = 0;
                if (maskRod.x1!=0)
                    offsetX = maskRod.x1;
                if (maskRod.y1!=0)
                    offsetY = maskRod.y1;
                image0.composite(mask,offsetX,offsetY,Magick::CopyOpacityCompositeOp);
            }
        }*/
        container.composite(image0,0,-spacing,Magick::OverCompositeOp);
        container.composite(image,0,mirrorHeight-offset,Magick::OverCompositeOp);
    }
    else
        container=image;

    // return image
    if (dstClip_ && dstClip_->isConnected()) {
        output.composite(container, 0, 0, Magick::OverCompositeOp);
#if MagickLibVersion >= 0x700
        output.composite(container, 0, 0, Magick::CopyAlphaCompositeOp);
#else
        output.composite(container, 0, 0, Magick::CopyOpacityCompositeOp);
#endif
        output.write(0,0,args.renderWindow.x2 - args.renderWindow.x1,args.renderWindow.y2 - args.renderWindow.y1,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
    }
}

bool ReflectionPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(ReflectionPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void ReflectionPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("Mirror/Reflection transform node.");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
    desc.setHostMaskingEnabled(true);
    desc.setHostMixingEnabled(true);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReflectionPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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

    // make pages and params
    PageParamDescriptor *page = desc.definePageParam("Controls");
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamOffset);
        param->setLabel(kParamOffsetLabel);
        param->setHint(kParamOffsetHint);
        param->setRange(-2000, 2000);
        param->setDisplayRange(-500, 500);
        param->setDefault(kParamOffsetDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamSpace);
        param->setLabel(kParamSpaceLabel);
        param->setHint(kParamSpaceHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamSpaceDefault);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamReflection);
        param->setLabel(kParamReflectionLabel);
        param->setHint(kParamReflectionHint);
        param->setDefault(kParamReflectionDefault);
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
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamMirror);
        param->setLabel(kParamMirrorLabel);
        param->setHint(kParamMirrorHint);
        param->appendOption("Undefined");
        param->appendOption("North");
        param->appendOption("South");
        param->appendOption("East");
        param->appendOption("West");
        param->appendOption("NorthWest");
        param->appendOption("NorthEast");
        param->appendOption("SouthWest");
        param->appendOption("SouthEast");
        param->appendOption("Flip");
        param->appendOption("Flop");
        param->appendOption("Flip+Flop");
        param->setDefault(kParamMirrorDefault);
        param->setAnimates(true);
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
ImageEffect* ReflectionPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new ReflectionPlugin(handle);
}

static ReflectionPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
