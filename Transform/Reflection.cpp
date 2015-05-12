/*

openfx-arena - https://github.com/olear/openfx-arena

Copyright (c) 2015, Ole-André Rodlie <olear@fxarena.net>
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

*  Neither the name of the {organization} nor the names of its
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

#include "Reflection.h"
#include <iostream>
#include "ofxsMacros.h"
#include <Magick++.h>

#define kPluginName "Reflection"
#define kPluginGrouping "Filter"
#define kPluginDescription  "Apply reflection effect to image."

#define kPluginIdentifier "net.fxarena.openfx.Reflection"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1 // does not work 100% in natron
#define kRenderThreadSafety eRenderInstanceSafe

#define kParamSpace "spacing"
#define kParamSpaceLabel "Space"
#define kParamSpaceHint "Space between image and reflection"
#define kParamSpaceDefault 0

#define kParamOffset "offset"
#define kParamOffsetLabel "Offset"
#define kParamOffsetHint "Mirror offset"
#define kParamOffsetDefault 0

using namespace OFX;

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
    OFX::Clip *maskClip_;
    OFX::Double2DParam *position_;
    OFX::IntParam *spacing_;
    OFX::IntParam *offset_;
};

ReflectionPlugin::ReflectionPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
, maskClip_(0)
{
    Magick::InitializeMagick("");

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    assert(!maskClip_ || maskClip_->getPixelComponents() == OFX::ePixelComponentAlpha);

    spacing_ = fetchIntParam(kParamSpace);
    offset_ = fetchIntParam(kParamOffset);
    assert(spacing_ && offset_);
}

ReflectionPlugin::~ReflectionPlugin()
{
}

/* Override the render */
void
ReflectionPlugin::render(const OFX::RenderArguments &args)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // Get src clip
    if (!srcClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(srcClip_);
    std::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    OFX::BitDepthEnum bitDepth = eBitDepthNone;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        bitDepth = srcImg->getPixelDepth();
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
    if (getContext() != OFX::eContextFilter && maskClip_ && maskClip_->isConnected())
        maskRod=maskImg->getRegionOfDefinition();

    // Get dst clip
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
    OfxRectI dstRod = dstImg->getRegionOfDefinition();

    // dst bit depth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && dstBitDepth != srcImg->getPixelDepth())) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // dst pixel components
    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if ((dstComponents != OFX::ePixelComponentRGBA && dstComponents != OFX::ePixelComponentRGB && dstComponents != OFX::ePixelComponentAlpha) ||
        (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }
    std::string channels;
    switch (dstComponents) {
    case ePixelComponentRGBA:
        channels = "RGBA";
        break;
    case ePixelComponentRGB:
        channels = "RGB";
        break;
    case ePixelComponentAlpha:
        channels = "A";
        break;
    }

    // are we in the image bounds?
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Get params
    int spacing,offset;
    spacing_->getValueAtTime(args.time, spacing);
    offset_->getValueAtTime(args.time, offset);

    // Setup image
    int width = 0;
    int height = 0;
    int maskWidth = 0;
    int maskHeight = 0;
    width = srcRod.x2-srcRod.x1;
    height = srcRod.y2-srcRod.y1;
    maskWidth = maskRod.x2-maskRod.x1;
    maskHeight = maskRod.y2-maskRod.y1;

    // Read image
    Magick::Image image(width,height,channels,Magick::FloatPixel,(float*)srcImg->getPixelData());
    Magick::Image wrapper(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));

    // proc image
    int mirrorWidth = width;
    int mirrorHeight = height/2;
    Magick::Image mirror;
    mirror = image;
    mirror.backgroundColor("none");
    mirror.flip();
    mirror.crop(Magick::Geometry(mirrorWidth,mirrorHeight-offset,0,offset+offset));
    image.crop(Magick::Geometry(mirrorWidth,mirrorHeight+offset,0,mirrorHeight-offset));

    // apply mask
    if (maskClip_ && maskClip_->isConnected() && maskWidth>0 && maskHeight>0) {
        Magick::Image mask(maskWidth,maskHeight,"A",Magick::FloatPixel,(float*)maskImg->getPixelData());
        mirror.composite(mask,0,0,Magick::CopyOpacityCompositeOp);
    }

    // comp image
    wrapper.composite(mirror,0,-spacing,Magick::OverCompositeOp);
    wrapper.composite(image,0,mirrorHeight-offset,Magick::OverCompositeOp);

    // return image
    switch (dstBitDepth) {
    case eBitDepthUByte:
        if (image.depth()>8)
            image.depth(8);
        wrapper.write(0,0,width,height,"RGBA",Magick::CharPixel,(float*)dstImg->getPixelData());
        break;
    case eBitDepthUShort:
        if (image.depth()>16)
            image.depth(16);
        wrapper.write(0,0,width,height,"RGBA",Magick::ShortPixel,(float*)dstImg->getPixelData());
        break;
    case eBitDepthFloat:
        wrapper.write(0,0,width,height,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
        break;
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
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthUByte);
    desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthFloat);

    // other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReflectionPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{   
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    //dstClip->addSupportedComponent(ePixelComponentRGB);
    dstClip->setSupportsTiles(kSupportsTiles);

    ClipDescriptor *maskClip = desc.defineClip("Mask");
    maskClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    maskClip->setTemporalClipAccess(false);
    maskClip->setOptional(true);
    maskClip->setSupportsTiles(kSupportsTiles);
    maskClip->setIsMask(true);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamOffset);
        param->setLabel(kParamOffsetLabel);
        param->setHint(kParamOffsetHint);
        param->setRange(0, 200);
        param->setDisplayRange(0, 200);
        param->setDefault(kParamOffsetDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamSpace);
        param->setLabel(kParamSpaceLabel);
        param->setHint(kParamSpaceHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamSpaceDefault);
        page->addChild(*param);
    }

}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReflectionPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new ReflectionPlugin(handle);
}

void getReflectionPluginID(OFX::PluginFactoryArray &ids)
{
    static ReflectionPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
