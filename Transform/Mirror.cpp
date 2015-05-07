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

#include "Mirror.h"

#include "ofxsMacros.h"
#include <Magick++.h>

#define kPluginName "Mirror"
#define kPluginGrouping "Transform"
#define kPluginDescription  "Mirror image."

#define kPluginIdentifier "net.fxarena.openfx.Mirror"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kParamMirror "mirror"
#define kParamMirrorLabel "Region"
#define kParamMirrorHint "Mirror image"

#define REGION_NORTH "North"
#define REGION_SOUTH "South"
#define REGION_EAST "East"
#define REGION_WEST "West"
#define REGION_NORTHWEST "NorthWest"
#define REGION_NORTHEAST "NorthEast"
#define REGION_SOUTHWEST "SouthWest"
#define REGION_SOUTHEAST "SouthEast"
#define REGION_FLIP "Flip"
#define REGION_FLOP "Flop"

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

using namespace OFX;

class MirrorPlugin : public OFX::ImageEffect
{
public:
    MirrorPlugin(OfxImageEffectHandle handle);
    virtual ~MirrorPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::ChoiceParam *mirror_;
};

MirrorPlugin::MirrorPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick("");
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    mirror_ = fetchChoiceParam(kParamMirror);
    assert(mirror_);
}

MirrorPlugin::~MirrorPlugin()
{
}

void MirrorPlugin::render(const OFX::RenderArguments &args)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

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

    // get bit depth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && dstBitDepth != srcImg->getPixelDepth())) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get pixel component
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

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // get param
    int mirror;
    mirror_->getValueAtTime(args.time, mirror);

    // read image
    Magick::Image image(srcRod.x2-srcRod.x1,srcRod.y2-srcRod.y1,channels,Magick::FloatPixel,(float*)srcImg->getPixelData());

    // proc image
    int magickWidth = srcRod.x2-srcRod.x1;
    int magickHeight = srcRod.y2-srcRod.y1;
    int mirrorWidth = magickWidth/2;
    int mirrorHeight = magickHeight/2;
    Magick::Image image1;
    Magick::Image image2;
    Magick::Image image3;
    Magick::Image image4;
    image1 = image;
    //image1.backgroundColor("none");
    switch(mirror) {
    case 1: // North
          image1.flip();
          image1.crop(Magick::Geometry(magickWidth,mirrorHeight,0,0));
          break;
    case 2: // South
          image.flip();
          image1.crop(Magick::Geometry(magickWidth,mirrorHeight,0,0));
          break;
    case 3: // East
          image1.flop();
          image1.crop(Magick::Geometry(mirrorWidth,magickHeight,0,0));
          break;
    case 4: // West
        image.flop();
        image1.crop(Magick::Geometry(mirrorWidth,magickHeight,0,0));
          break;
    case 5: // NorthWest
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,0,mirrorHeight));
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 6: // NorthEast
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
    case 9: // Flip
        image.flip();
        break;
    case 10: // Flop
        image.flop();
        break;
    }
    if (mirror==5||mirror==6||mirror==7||mirror==8) {
        image.composite(image1,0,mirrorHeight,Magick::OverCompositeOp);
        image.composite(image2,mirrorWidth,mirrorHeight,Magick::OverCompositeOp);
        image.composite(image3,mirrorWidth,0,Magick::OverCompositeOp);
        image.composite(image4,0,0,Magick::OverCompositeOp);
    }
    else {
        if (mirror==1||mirror==2||mirror==3||mirror==4)
            image.composite(image1,0,0,Magick::OverCompositeOp);
    }

    // check bit depth
    switch (bitDepth) {
    case OFX::eBitDepthUByte:
        if (image.depth()>8)
            image.depth(8);
        break;
    case OFX::eBitDepthUShort:
        if (image.depth()>16)
            image.depth(16);
        break;
    }

    // return image
    image.write(0,0,dstRod.x2-dstRod.x1,dstRod.y2-dstRod.y1,channels,Magick::FloatPixel,(float*)dstImg->getPixelData());
}

bool MirrorPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(MirrorPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void MirrorPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    //desc.addSupportedBitDepth(eBitDepthUByte); // not tested yet
    //desc.addSupportedBitDepth(eBitDepthUShort); // not tested yet
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void MirrorPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    //srcClip->addSupportedComponent(ePixelComponentAlpha); // should work, not tested
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentRGB);
    //dstClip->addSupportedComponent(ePixelComponentAlpha); // should work, not tested
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages and to things in
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamMirror);
        param->setLabel(kParamMirrorLabel);
        param->setHint(kParamMirrorHint);
	param->appendOption("None");
        param->appendOption(REGION_NORTH);
        param->appendOption(REGION_SOUTH);
        param->appendOption(REGION_EAST);
        param->appendOption(REGION_WEST);
        param->appendOption(REGION_NORTHWEST);
        param->appendOption(REGION_NORTHEAST);
        param->appendOption(REGION_SOUTHWEST);
        param->appendOption(REGION_SOUTHEAST);
        param->appendOption(REGION_FLIP);
        param->appendOption(REGION_FLOP);
        param->setAnimates(true);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* MirrorPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new MirrorPlugin(handle);
}


void getMirrorPluginID(OFX::PluginFactoryArray &ids)
{
    static MirrorPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
