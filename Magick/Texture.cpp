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

#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include "ofxNatron.h"
#include <Magick++.h>
#include <iostream>

#define kPluginName "TextureOFX"
#define kPluginGrouping "Extra/Draw"
#define kPluginIdentifier "net.fxarena.openfx.Texture"
#define kPluginVersionMajor 3
#define kPluginVersionMinor 7

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 0
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

#define kParamEffect "background"
#define kParamEffectLabel "Background"
#define kParamEffectHint "Background type"
#define kParamEffectDefault 6

#define kParamSeed "seed"
#define kParamSeedLabel "Seed"
#define kParamSeedHint "Seed the random generator"
#define kParamSeedDefault 0

#define kParamWidth "width"
#define kParamWidthLabel "Width"
#define kParamWidthHint "Set canvas width, default (0) is project format"
#define kParamWidthDefault 0

#define kParamHeight "height"
#define kParamHeightLabel "Height"
#define kParamHeightHint "Set canvas height, default (0) is project format"
#define kParamHeightDefault 0

#define kParamFromColor "fromColor"
#define kParamFromColorLabel "Color from"
#define kParamFromColorHint "Set start color, you must set a end color for this to work. Valid values are: none (transparent), color name (red, blue etc) or hex colors"

#define kParamToColor "toColor"
#define kParamToColorLabel "Color to"
#define kParamToColorHint "Set end color, you must set a start color for this to work. Valid values are : none (transparent), color name (red, blue etc) or hex colors"

#define kParamOpenMP "openmp"
#define kParamOpenMPLabel "OpenMP"
#define kParamOpenMPHint "Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host."
#define kParamOpenMPDefault false

using namespace OFX;
static bool gHostIsNatron = false;
static bool _hasOpenMP = false;

static unsigned int hash(unsigned int a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

class TexturePlugin : public OFX::ImageEffect
{
public:
    TexturePlugin(OfxImageEffectHandle handle);
    virtual ~TexturePlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
    virtual void getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::ChoiceParam *effect_;
    OFX::IntParam *seed_;
    OFX::IntParam *width_;
    OFX::IntParam *height_;
    OFX::StringParam *fromColor_;
    OFX::StringParam *toColor_;
    OFX::BooleanParam *enableOpenMP_;
};

TexturePlugin::TexturePlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(NULL)
, effect_(NULL)
, seed_(NULL)
, width_(NULL)
, height_(NULL)
, fromColor_(NULL)
, toColor_(NULL)
, enableOpenMP_(NULL)
{
    Magick::InitializeMagick(NULL);

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    effect_ = fetchChoiceParam(kParamEffect);
    seed_ = fetchIntParam(kParamSeed);
    width_ = fetchIntParam(kParamWidth);
    height_ = fetchIntParam(kParamHeight);
    fromColor_ = fetchStringParam(kParamFromColor);
    toColor_ = fetchStringParam(kParamToColor);
    enableOpenMP_ = fetchBooleanParam(kParamOpenMP);

    assert(effect_ && seed_ && width_ && height_ && fromColor_ && toColor_ && enableOpenMP_);
}

TexturePlugin::~TexturePlugin()
{
}

/* set the frame varying flag */
void TexturePlugin::getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences)
{
    clipPreferences.setOutputFrameVarying(true);
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

    OFX::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
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
    bool enableOpenMP = false;
    std::string fromColor, toColor;
    effect_->getValueAtTime(args.time, effect);
    seed_->getValueAtTime(args.time, seed);
    fromColor_->getValueAtTime(args.time, fromColor);
    toColor_->getValueAtTime(args.time, toColor);
    enableOpenMP_->getValueAtTime(args.time, enableOpenMP);

    // OpenMP
#ifndef LEGACYIM
    unsigned int threads = 1;
    if (_hasOpenMP && enableOpenMP)
        threads = OFX::MultiThread::getNumCPUs();

    Magick::ResourceLimits::thread(threads);
#endif

    // Generate empty image
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));

    // Set seed
#ifndef NOMAGICKSEED
    Magick::SetRandomSeed(hash((unsigned)(args.time)^seed));
#endif

    // generate background
    try {
        switch (effect) {
        case 0: // Plasma
            if (fromColor.empty() && toColor.empty())
                image.read("plasma:");
            else
                image.read("plasma:"+fromColor+"-"+toColor);
            break;
        case 1: // Plasma Fractal
            image.read("plasma:fractal");
            break;
        case 2: // GaussianNoise
            image.addNoise(Magick::GaussianNoise);
            break;
        case 3: // ImpulseNoise
            image.addNoise(Magick::ImpulseNoise);
            break;
        case 4: // LaplacianNoise
            image.addNoise(Magick::LaplacianNoise);
            break;
        case 5: // checkerboard
            image.read("pattern:checkerboard");
            break;
        case 6: // stripes
            image.extent(Magick::Geometry(width,1));
            image.addNoise(Magick::GaussianNoise);
            image.channel(Magick::GreenChannel);
            image.negate();
            image.scale(Magick::Geometry(width,height));
            break;
        case 7: // gradient
            if (fromColor.empty() && toColor.empty())
                image.read("gradient:");
            else
                image.read("gradient:"+fromColor+"-"+toColor);
            break;
        case 8: // radial-gradient
            if (fromColor.empty() && toColor.empty())
                image.read("radial-gradient:");
            else
                image.read("radial-gradient:"+fromColor+"-"+toColor);
            break;
        case 9: // loops1
            image.addNoise(Magick::GaussianNoise);
            break;
        case 10: // loops2
            image.addNoise(Magick::ImpulseNoise);
            break;
        case 11: // loops3
            image.addNoise(Magick::LaplacianNoise);
            break;
        }
        if (effect>8 && effect<12) { // loops 1 2 3
#if MagickLibVersion >= 0x700
            image.alpha(false);
#else
            image.matte(false);
#endif
            image.blur(0,10);
            image.normalize();
            image.fx("sin(u*4*pi)*100");
            image.edge(1);
            image.blur(0,10);
#if MagickLibVersion >= 0x700
            image.alpha(true);
#else
            image.matte(true);
#endif
        }
    }
    catch(Magick::Warning &warning) { // ignore since warns interupt render
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
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
    desc.setPluginDescription("Texture/Background generator node.");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TexturePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{   
    std::string features = MagickCore::GetMagickFeatures();
    if (features.find("OpenMP") != std::string::npos)
        _hasOpenMP = true;

    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

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
    PageParamDescriptor *page = desc.definePageParam("Controls");
    GroupParamDescriptor *groupCanvas = desc.defineGroupParam("Canvas");
    groupCanvas->setOpen(false);
    {
        page->addChild(*groupCanvas);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamEffect);
        param->setLabel(kParamEffectLabel);
        param->setHint(kParamEffectHint);
        if (gHostIsNatron) {
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);
            param->appendOption("Plasma/Regular");
            param->appendOption("Plasma/Fractal");
            param->appendOption("Noise/Gaussian");
            param->appendOption("Noise/Impulse");
            param->appendOption("Noise/Laplacian");
            param->appendOption("Misc/Checkerboard");
            param->appendOption("Misc/Stripes");
            param->appendOption("Gradient/Regular");
            param->appendOption("Gradient/Radial");
            param->appendOption("Misc/Loops 1");
            param->appendOption("Misc/Loops 2");
            param->appendOption("Misc/Loops 3");
        }
        else {
            param->appendOption("Plasma");
            param->appendOption("Plasma Fractal");
            param->appendOption("GaussianNoise");
            param->appendOption("ImpulseNoise");
            param->appendOption("LaplacianNoise");
            param->appendOption("Checkerboard");
            param->appendOption("Stripes");
            param->appendOption("Gradient");
            param->appendOption("Gradient Radial");
            param->appendOption("Loops 1");
            param->appendOption("Loops 2");
            param->appendOption("Loops 3");
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
        param->setParent(*groupCanvas);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamHeight);
        param->setLabel(kParamHeightLabel);
        param->setHint(kParamHeightHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamHeightDefault);
        param->setParent(*groupCanvas);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamFromColor);
        param->setLabel(kParamFromColorLabel);
        param->setHint(kParamFromColorHint);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamToColor);
        param->setLabel(kParamToColorLabel);
        param->setHint(kParamToColorHint);
        param->setStringType(eStringTypeSingleLine);
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
ImageEffect* TexturePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TexturePlugin(handle);
}

static TexturePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
