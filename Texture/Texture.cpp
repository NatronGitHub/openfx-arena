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

#include "Texture.h"
#include "ofxsMacros.h"
#include "ofxNatron.h"
#include <Magick++.h>

#define kPluginName "Texture"
#define kPluginGrouping "Draw"

#define kPluginIdentifier "net.fxarena.openfx.Texture"
#define kPluginVersionMajor 3
#define kPluginVersionMinor 1

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 0
#define kRenderThreadSafety eRenderInstanceSafe

#define kParamEffect "background"
#define kParamEffectLabel "Background"
#define kParamEffectHint "Background type"
#define kParamEffectDefault 6

#define kParamSeed "seed"
#define kParamSeedLabel "Seed"
#define kParamSeedHint "Seed the random generator"
#define kParamSeedDefault 4321

#define kParamWidth "width"
#define kParamWidthLabel "Width"
#define kParamWidthHint "Set canvas width, default (0) is project format"
#define kParamWidthDefault 0

#define kParamHeight "height"
#define kParamHeightLabel "Height"
#define kParamHeightHint "Set canvas height, default (0) is project format"
#define kParamHeightDefault 0

using namespace OFX;
static bool gHostIsNatron = false;

class TexturePlugin : public OFX::ImageEffect
{
public:
    TexturePlugin(OfxImageEffectHandle handle);
    virtual ~TexturePlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::ChoiceParam *effect_;
    OFX::IntParam *seed_;
    OFX::IntParam *width_;
    OFX::IntParam *height_;
};

TexturePlugin::TexturePlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, width_(0)
, height_(0)
{
    Magick::InitializeMagick(NULL);

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    effect_ = fetchChoiceParam(kParamEffect);
    seed_ = fetchIntParam(kParamSeed);
    width_ = fetchIntParam(kParamWidth);
    height_ = fetchIntParam(kParamHeight);

    assert(effect_ && seed_ && width_ && height_);
}

TexturePlugin::~TexturePlugin()
{
}

/* Override the render */
void TexturePlugin::render(const OFX::RenderArguments &args)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
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
    if (dstBitDepth != OFX::eBitDepthFloat && dstBitDepth != OFX::eBitDepthUShort && dstBitDepth != OFX::eBitDepthUByte) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if ((dstComponents != OFX::ePixelComponentRGBA && dstComponents != OFX::ePixelComponentRGB && dstComponents != OFX::ePixelComponentAlpha)) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    OfxRectI dstRod = dstImg->getRegionOfDefinition();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Get params
    int effect,seed;
    effect_->getValueAtTime(args.time, effect);
    seed_->getValueAtTime(args.time, seed);

    // Generate empty image
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));

    // Set seed
    if (seed!=0)
        Magick::SetRandomSeed(seed);

    // generate background
    switch (effect) {
    case 0: // Plasma
        image.read("plasma:");
        break;
    case 1: // Plasma
        image.read("plasma:grey-grey");
        break;
    case 2: // Plasma
        image.read("plasma:white-blue");
        break;
    case 3: // Plasma
        image.read("plasma:green-yellow");
        break;
    case 4: // Plasma
        image.read("plasma:red-blue");
        break;
    case 5: // Plasma
        image.read("plasma:tomato-steelblue");
        break;
    case 6: // Plasma Fractal
        image.read("plasma:fractal");
        break;
    case 7: // GaussianNoise
        image.addNoise(Magick::GaussianNoise);
        break;
    case 8: // ImpulseNoise
        image.addNoise(Magick::ImpulseNoise);
        break;
    case 9: // LaplacianNoise
        image.addNoise(Magick::LaplacianNoise);
        break;
    case 10: // checkerboard
        image.read("pattern:checkerboard");
        break;
    case 11: // stripes
        image.extent(Magick::Geometry(width,1));
        image.addNoise(Magick::GaussianNoise);
        image.channel(Magick::GreenChannel);
        image.negate();
        image.scale(Magick::Geometry(width,height));
        break;
    case 12:
        image.read("gradient:");
        break;
    case 13:
        image.read("gradient:blue");
        break;
    case 14:
        image.read("gradient:yellow");
        break;
    case 15:
        image.read("gradient:green-yellow");
        break;
    case 16:
        image.read("gradient:red-blue");
        break;
    case 17:
        image.read("gradient:tomato-steelblue");
        break;
    case 18:
        image.read("gradient:snow-navy");
        break;
    case 19:
        image.read("gradient:gold-firebrick");
        break;
    case 20:
        image.read("gradient:yellow-limegreen");
        break;
    case 21:
        image.read("gradient:khaki-tomato");
        break;
    case 22:
        image.read("gradient:darkcyan-snow");
        break;
    case 23:
        image.read("gradient:none-firebrick");
        break;
    case 24:
        image.read("gradient:none-yellow");
        break;
    case 25:
        image.read("gradient:none-steelblue");
        break;
    case 26:
        image.read("gradient:none-navy");
        break;
    case 27:
        image.read("gradient:none-gold");
        break;
    case 28:
        image.read("gradient:none-orange");
        break;
    case 29:
        image.read("gradient:none-tomato");
        break;
    case 30:
        image.read("radial-gradient:");
        break;
    case 31:
        image.read("radial-gradient:blue");
        break;
    case 32:
        image.read("radial-gradient:yellow");
        break;
    case 33:
        image.read("radial-gradient:green-yellow");
        break;
    case 34:
        image.read("radial-gradient:red-blue");
        break;
    case 35:
        image.read("radial-gradient:tomato-steelblue");
        break;
    }

    // return image
    if (dstClip_ && dstClip_->isConnected())
        image.write(0,0,width,height,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
}

bool TexturePlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }

    int width,height;
    width_->getValue(width);
    height_->getValue(height);

    if (width>0 && height>0) {
        rod.x1 = rod.y1 = 0;
        rod.x2 = width;
        rod.y2 = height;
    }
    else {
        rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
    }

    return true;
}

mDeclarePluginFactory(TexturePluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void TexturePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    std::string magickV = MagickCore::GetMagickVersion(NULL);
    std::string delegates = MagickCore::GetMagickDelegates();
    desc.setPluginDescription("Texture/Background generator for Natron.\n\nWritten by Ole-André Rodlie <olear@fxarena.net>\n\n Powered by "+magickV+"\n\nFeatures: "+delegates);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TexturePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{   
    gHostIsNatron = (OFX::getImageEffectHostDescription()->hostName == kNatronOfxHostName);

    // there has to be an input clip, even for generators
    ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setOptional(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamEffect);
        param->setLabel(kParamEffectLabel);
        param->setHint(kParamEffectHint);
        if (gHostIsNatron) {
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);
            param->appendOption("Plasma/Regular");
            param->appendOption("Plasma/grey-grey");
            param->appendOption("Plasma/white-blue");
            param->appendOption("Plasma/green-yellow");
            param->appendOption("Plasma/red-blue");
            param->appendOption("Plasma/tomato-steelblue");
            param->appendOption("Plasma/Fractal");
            param->appendOption("Noise/Gaussian");
            param->appendOption("Noise/Impulse");
            param->appendOption("Noise/Laplacian");
            param->appendOption("Misc/Checkerboard");
            param->appendOption("Misc/Stripes");
            param->appendOption("Gradient/Regular");
            param->appendOption("Gradient/Blue");
            param->appendOption("Gradient/Yellow");
            param->appendOption("Gradient/green-yellow");
            param->appendOption("Gradient/red-blue");
            param->appendOption("Gradient/tomato-steelblue");
            param->appendOption("Gradient/snow-navy");
            param->appendOption("Gradient/gold-firebrick");
            param->appendOption("Gradient/yellow-limegreen");
            param->appendOption("Gradient/khaki-tomato");
            param->appendOption("Gradient/darkcyan-snow");
            param->appendOption("Gradient/none-firebrick");
            param->appendOption("Gradient/none-yellow");
            param->appendOption("Gradient/none-steelblue");
            param->appendOption("Gradient/none-navy");
            param->appendOption("Gradient/none-gold");
            param->appendOption("Gradient/none-orange");
            param->appendOption("Gradient/none-tomato");
            param->appendOption("Gradient/Radial/Regular");
            param->appendOption("Gradient/Radial/Blue");
            param->appendOption("Gradient/Radial/Yellow");
            param->appendOption("Gradient/Radial/green-yellow");
            param->appendOption("Gradient/Radial/red-blue");
            param->appendOption("Gradient/Radial/tomato-steelblue");
        }
        else {
            param->appendOption("Plasma");
            param->appendOption("Plasma grey-grey");
            param->appendOption("Plasma white-blue");
            param->appendOption("Plasma green-yellow");
            param->appendOption("Plasma red-blue");
            param->appendOption("Plasma tomato-steelblue");
            param->appendOption("Plasma Fractal");
            param->appendOption("GaussianNoise");
            param->appendOption("ImpulseNoise");
            param->appendOption("LaplacianNoise");
            param->appendOption("Checkerboard");
            param->appendOption("Stripes");
            param->appendOption("Gradient");
            param->appendOption("Gradient blue");
            param->appendOption("Gradient yellow");
            param->appendOption("Gradient green-yellow");
            param->appendOption("Gradient red-blue");
            param->appendOption("Gradient tomato-steelblue");
            param->appendOption("Gradient snow-navy");
            param->appendOption("Gradient gold-firebrick");
            param->appendOption("Gradient yellow-limegreen");
            param->appendOption("Gradient khaki-tomato");
            param->appendOption("Gradient darkcyan-snow");
            param->appendOption("Gradient none-firebrick");
            param->appendOption("Gradient none-yellow");
            param->appendOption("Gradient none-steelblue");
            param->appendOption("Gradient none-navy");
            param->appendOption("Gradient none-gold");
            param->appendOption("Gradient none-orange");
            param->appendOption("Gradient none-tomato");
            param->appendOption("Gradient Radial");
            param->appendOption("Gradient Radial blue");
            param->appendOption("Gradient Radial yellow");
            param->appendOption("Gradient Radial green-yellow");
            param->appendOption("Gradient Radial red-blue");
            param->appendOption("Gradient Radial tomato-steelblue");
        }
        param->setDefault(kParamEffectDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamSeed);
        param->setLabel(kParamSeedLabel);
        param->setHint(kParamSeedHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 5000);
        param->setDefault(kParamSeedDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamWidth);
        param->setLabel(kParamWidthLabel);
        param->setHint(kParamWidthHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamWidthDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamHeight);
        param->setLabel(kParamHeightLabel);
        param->setHint(kParamHeightHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamHeightDefault);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TexturePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TexturePlugin(handle);
}

void getTexturePluginID(OFX::PluginFactoryArray &ids)
{
    static TexturePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
