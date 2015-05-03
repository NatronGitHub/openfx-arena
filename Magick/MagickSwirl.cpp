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

#include "MagickSwirl.h"
#include <iostream>
#include "ofxsMacros.h"
#include <Magick++.h>

#define kPluginName "Swirl"
#define kPluginGrouping "Filter"
#define kPluginDescription  "Swirl image."

#define kPluginIdentifier "net.fxarena.openfx.MagickSwirl"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

#define kParamSwirl "swirl"
#define kParamSwirlLabel "Swirl"
#define kParamSwirlHint "Swirl image by degree"
#define kParamSwirlDefault 0

using namespace OFX;

class MagickSwirlPlugin : public OFX::ImageEffect
{
public:
    MagickSwirlPlugin(OfxImageEffectHandle handle);
    virtual ~MagickSwirlPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;

private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;

    // Swirl degree param
    OFX::DoubleParam *swirlDegree_;
};

MagickSwirlPlugin::MagickSwirlPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick("");

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    swirlDegree_ = fetchDoubleParam(kParamSwirl);
    assert(swirlDegree_);
}

MagickSwirlPlugin::~MagickSwirlPlugin()
{
}

/* Override the render */
void
MagickSwirlPlugin::render(const OFX::RenderArguments &args)
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
    if (srcImg.get()) {
        if (srcImg->getRenderScale().x != args.renderScale.x ||
            srcImg->getRenderScale().y != args.renderScale.y ||
            srcImg->getField() != args.fieldToRender) {
            setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
            OFX::throwSuiteStatusException(kOfxStatFailed);
            return;
        }
    }

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
    int channels = 0;
    if (dstComponents != OFX::ePixelComponentRGB)
        channels = 4;
    else
        channels = 3;

    // are we in the image bounds?
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Get params
    double swirl;
    swirlDegree_->getValueAtTime(args.time, swirl);

    // tmp debug
    std::cout << "\n\y1: " << dstBounds.y1 << "\n";
    std::cout << "y2: " << dstBounds.y2 << "\n";
    std::cout << "x1: " << dstBounds.x1 << "\n";
    std::cout << "x2: " << dstBounds.x2 << "\n\n";

    // Setup image
    int width = 0;
    int height = 0;
    int offsetX = 0;
    int offsetY = 0;
    width=dstBounds.x2-dstBounds.x1;
    height=dstBounds.y2-dstBounds.y1;

    std::string colorType;
    if (channels==4)
        colorType="RGBA";
    else
        colorType = "RGB";

    // Read image
    Magick::Image image(width,height,colorType,Magick::FloatPixel,(float*)srcImg->getPixelData());

    // tmp debug
    image.write("/tmp/debug.png");
    std::cout << "input width: " << width << "(x2-x1) height: " << height << "(y2-y1)\n";

    // Swirl image
    if (swirl!=0)
        image.swirl(swirl);

    // adjust to RoD
    if (dstBounds.x1<0) {
        width = width-(dstBounds.x1*-1);
        offsetX = dstBounds.x1*-1;
    }
    else if (dstBounds.x1>0) { // TODO!!!
        //width = width+dstBounds.x1;
        //offsetX = dstBounds.x1;
    }

    if(dstBounds.y1<0) {
        height = height-(dstBounds.y1*-1);
        offsetY = dstBounds.y1*-1;
    }
    else if (dstBounds.y1>0) { // TODO!!!
        //height = height+dstBounds.y1;
        //offsetY = dstBounds.y1;
    }

    // tmp debug
    std::cout << "tmp width: " << width << " height: " << height << " offset: " << offsetX << "x" << offsetY << "\n";

    // Return image
    int widthStep = width*channels;
    int imageSize = width*height*channels;
    float* block;
    block = new float[imageSize];
    image.write(offsetX,offsetY,width,height,colorType,Magick::FloatPixel,block);
    if (channels==4) { // RGBA
        for(int y = 0; y<height; y++) {
            OfxRGBAColourF *dstPix = (OfxRGBAColourF*)dstImg->getPixelAddress(0,y);
            float *srcPix = (float*)(block+y*widthStep);
            for(int x = 0; x<width; x++) {
                dstPix->r = srcPix[0];
                dstPix->g = srcPix[1];
                dstPix->b = srcPix[2];
                dstPix->a = srcPix[3];
                dstPix++;
                srcPix+=channels;
            }
        }
    }
    else { // RGB
        for(int y = 0; y<height; y++) {
            OfxRGBColourF *dstPix = (OfxRGBColourF*)dstImg->getPixelAddress(0,y);
            float *srcPix = (float*)(block+y*widthStep);
            for(int x = 0; x<width; x++) {
                dstPix->r = srcPix[0];
                dstPix->g = srcPix[1];
                dstPix->b = srcPix[2];
                dstPix++;
                srcPix+=channels;
            }
        }
    }
    free(block);
}

mDeclarePluginFactory(MagickSwirlPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void MagickSwirlPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void MagickSwirlPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{   
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    //srcClip->setTemporalClipAccess(false); // ???
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentRGB);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kParamSwirlLabel);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSwirl);
        param->setLabel(kParamSwirlLabel);
        param->setHint(kParamSwirlHint);
        param->setRange(-360, 360);
        param->setDisplayRange(-360, 360);
        param->setDefault(kParamSwirlDefault);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* MagickSwirlPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new MagickSwirlPlugin(handle);
}

void getMagickSwirlPluginID(OFX::PluginFactoryArray &ids)
{
    static MagickSwirlPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
