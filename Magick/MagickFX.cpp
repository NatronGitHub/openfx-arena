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

#include "MagickFX.h"
#include <iostream>
#include "ofxsProcessing.H"
#include "ofxsCopier.h"
#include "ofxsPositionInteract.h"
#include "ofxNatron.h"
#include "ofxsMacros.h"
#include <Magick++.h>

#define kPluginName "FX"
#define kPluginGrouping "Filter"
#define kPluginDescription  "Apply Special Effects with an Fx Expression.\n\nhttp://www.imagemagick.org/script/fx.php"

#define kPluginIdentifier "net.fxarena.openfx.MagickFX"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0 // ???

#define kSupportsMultiResolution 1 // ???
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

#define kParamFX "fx"
#define kParamFXLabel "FX"
#define kParamFXHint "Apply Special Effects with an Fx Expression.\n\nhttp://www.imagemagick.org/script/fx.php"

#define kParamFXChannel "fxC"
#define kParamFXChannelLabel "FX Channel"
#define kParamFXChannelHint "Apply FX to channel(s)"

using namespace OFX;

class MagickFXPlugin : public OFX::ImageEffect
{
public:

    MagickFXPlugin(OfxImageEffectHandle handle);

    virtual ~MagickFXPlugin();

    /* Override the render */
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;

    /* override is identity */
    virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime) OVERRIDE FINAL;

    /* override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;

    /* override changed clip */
    //virtual void changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName) OVERRIDE FINAL;

    // override the rod call
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;

private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;

    // param
    OFX::StringParam *fxText_;
    OFX::ChoiceParam *fxChannel_;
};

MagickFXPlugin::MagickFXPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick("");

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    fxText_ = fetchStringParam(kParamFX);
    fxChannel_ = fetchChoiceParam(kParamFXChannel);
    assert(fxText_&&fxChannel_);
}

MagickFXPlugin::~MagickFXPlugin()
{
}

/* Override the render */
void
MagickFXPlugin::render(const OFX::RenderArguments &args)
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
    if (srcImg.get()) {
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

    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && dstBitDepth != srcImg->getPixelDepth())) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if ((dstComponents != OFX::ePixelComponentRGBA && dstComponents != OFX::ePixelComponentRGB && dstComponents != OFX::ePixelComponentAlpha) ||
        (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Get param
    std::string fx;
    fxText_->getValueAtTime(args.time, fx);
    int fxC;
    fxChannel_->getValueAtTime(args.time, fxC);

    // Read image
    int magickWidth = args.renderWindow.x2 - args.renderWindow.x1;
    int magickHeight = args.renderWindow.y2 - args.renderWindow.y1;
    int magickWidthStep = magickWidth*4;
    int magickSize = magickWidth*magickHeight*4;
    float* magickBlock;
    magickBlock = new float[magickSize];
    Magick::Image magickImage(magickWidth,magickHeight,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());

    try {
        // Apply fx
        switch(fxC) {
        case 1: // R
            magickImage.fx(fx,Magick::RedChannel);
            break;
        case 2: // G
            magickImage.fx(fx,Magick::GreenChannel);
            break;
        case 3: // B
            magickImage.fx(fx,Magick::BlueChannel);
            break;
        case 4: // A
            magickImage.fx(fx,Magick::MatteChannel);
            break;
        default: // RGBA
            magickImage.fx(fx);
            break;
        }
        // Write to buffer
        magickImage.write(0,0,magickWidth,magickHeight,"RGBA",Magick::FloatPixel,magickBlock);
    }
    catch(Magick::Error &error_) {
        std::cout << " MagickFX error " << error_.what() << "\n";
    }

    // Return image
    for(int y = args.renderWindow.y1; y < (args.renderWindow.y1 + magickHeight); y++) {
        OfxRGBAColourF *dstPix = (OfxRGBAColourF *)dstImg->getPixelAddress(args.renderWindow.x1, y);
        float *srcPix = (float*)(magickBlock + y * magickWidthStep + args.renderWindow.x1);
        for(int x = args.renderWindow.x1; x < (args.renderWindow.x1 + magickWidth); x++) {
            dstPix->r = srcPix[0];
            dstPix->g = srcPix[1];
            dstPix->b = srcPix[2];
            dstPix->a = srcPix[3];
            dstPix++;
            srcPix+=4;
        }
    }
    free(magickBlock);
}

bool MagickFXPlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &/*identityTime*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }
    return true;
}

void
MagickFXPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &/*paramName*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    clearPersistentMessage();
}

bool
MagickFXPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }
    if (srcClip_ && srcClip_->isConnected())
        rod = srcClip_->getRegionOfDefinition(args.time);
    else {
        rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
    }
    return true;
}

mDeclarePluginFactory(MagickFXPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void MagickFXPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void MagickFXPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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
    dstClip->addSupportedComponent(ePixelComponentRGB);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam("FX");
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamFX);
        param->setLabel(kParamFXLabel);
        param->setHint(kParamFXHint);
        param->setStringType(eStringTypeMultiLine);
        param->setAnimates(true);
        param->setDefault("u*1.5");
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamFXChannel);
        param->setLabel(kParamFXChannelLabel);
        param->setHint(kParamFXChannelHint);

        param->appendOption("RGBA");
        param->appendOption("R");
        param->appendOption("G");
        param->appendOption("B");
        param->appendOption("A");

        param->setAnimates(true);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* MagickFXPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new MagickFXPlugin(handle);
}

void getMagickFXPluginID(OFX::PluginFactoryArray &ids)
{
    static MagickFXPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
