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
#include "ofxsGenerator.h"
#include "ofxNatron.h"
#include <Magick++.h>
#include <iostream>

#define kPluginName "TextureOFX"
#define kPluginGrouping "Extra/Draw"
#define kPluginIdentifier "net.fxarena.openfx.Texture"
#define kPluginVersionMajor 4
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 0
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kSupportsMultipleClipPARs false
#define kSupportsMultipleClipDepths false

#define kParamEffect "background"
#define kParamEffectLabel "Background"
#define kParamEffectHint "Background type"
#define kParamEffectDefault 6

#define kParamSeed "seed"
#define kParamSeedLabel "Seed"
#define kParamSeedHint "Seed the random generator"
#define kParamSeedDefault 0

#define kParamFromColor "fromColor"
#define kParamFromColorLabel "Color from"
#define kParamFromColorHint "Set start color, you must set a end color for this to work. Valid values are: none (transparent), color name (red, blue etc) or hex colors"

#define kParamToColor "toColor"
#define kParamToColorLabel "Color to"
#define kParamToColorHint "Set end color, you must set a start color for this to work. Valid values are : none (transparent), color name (red, blue etc) or hex colors"

using namespace OFX;
static bool gHostIsNatron = false;

static unsigned int hash(unsigned int a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

class TexturePlugin : public GeneratorPlugin
{
public:
    TexturePlugin(OfxImageEffectHandle handle);
    virtual void render(const RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
    virtual void getClipPreferences(ClipPreferencesSetter &clipPreferences) OVERRIDE FINAL;
private:
    Clip *_dstClip;
    ChoiceParam *_effect;
    IntParam *_seed;
    StringParam *_fromColor;
    StringParam *_toColor;
};

TexturePlugin::TexturePlugin(OfxImageEffectHandle handle) :
    GeneratorPlugin(handle, false, false, false, false, true)
    , _dstClip(nullptr)
    , _effect(nullptr)
    , _seed(nullptr)
    , _fromColor(nullptr)
    , _toColor(nullptr)
{
    Magick::InitializeMagick(nullptr);
    Magick::ResourceLimits::thread(1);

    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    assert(_dstClip && (_dstClip->getPixelComponents() == ePixelComponentRGBA || _dstClip->getPixelComponents() == ePixelComponentRGB));

    _effect = fetchChoiceParam(kParamEffect);
    _seed = fetchIntParam(kParamSeed);
    _fromColor = fetchStringParam(kParamFromColor);
    _toColor = fetchStringParam(kParamToColor);
    assert(_effect && _seed && _fromColor && _toColor);
}


/* set the frame varying flag */
void TexturePlugin::getClipPreferences(ClipPreferencesSetter &clipPreferences)
{
    clipPreferences.setOutputFrameVarying(true);
    GeneratorPlugin::getClipPreferences(clipPreferences);
}

/* Override the render */
void TexturePlugin::render(const RenderArguments &args)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (!_dstClip) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(_dstClip);

    OFX::auto_ptr<OFX::Image> dstImg(_dstClip->fetchImage(args.time));
    if (!dstImg.get()) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    checkBadRenderScaleOrField(dstImg, args);

    BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != eBitDepthFloat && dstBitDepth != eBitDepthUShort && dstBitDepth != eBitDepthUByte) {
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != ePixelComponentRGBA) {
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    OfxRectI dstRod = dstImg->getRegionOfDefinition();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Get params
    int effect,seed;
    std::string fromColor, toColor;
    _effect->getValueAtTime(args.time, effect);
    _seed->getValueAtTime(args.time, seed);
    _fromColor->getValueAtTime(args.time, fromColor);
    _toColor->getValueAtTime(args.time, toColor);

    // Generate empty image
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    //std::cout << "WIDTH: " << width << " HEIGHT: " << height << std::endl;
    Magick::Image image( Magick::Geometry(width, height), Magick::Color("rgba(0, 0, 0, 0)") );

    // Set seed
#ifndef NOMAGICKSEED
    Magick::SetRandomSeed( hash((unsigned)(args.time)^seed) );
#endif

    // generate background
    try {
        switch (effect) {
        case 0: // Plasma
            if (fromColor.empty() && toColor.empty()) {
                image.read("plasma:");
            } else {
                image.read("plasma:"+fromColor+"-"+toColor);
            }
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
            image.extent( Magick::Geometry(width, 1) );
            image.addNoise(Magick::GaussianNoise);
            image.channel(Magick::GreenChannel);
            image.negate();
            image.scale( Magick::Geometry(width, height) );
            break;
        case 7: // gradient
            if ( fromColor.empty() && toColor.empty() ) {
                image.read("gradient:");
            } else {
                image.read("gradient:"+fromColor+"-"+toColor);
            }
            break;
        case 8: // radial-gradient
            if ( fromColor.empty() && toColor.empty() ) {
                image.read("radial-gradient:");
            } else {
                image.read("radial-gradient:"+fromColor+"-"+toColor);
            }
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
    catch(Magick::Warning &warning) { // ignore
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
    }

    // return image
    if (_dstClip && _dstClip->isConnected()) {
        switch (dstBitDepth) {
        case eBitDepthUByte:
            image.write( 0, 0, width, height, "RGBA", Magick::CharPixel, (char*)dstImg->getPixelData() );
            break;
        case eBitDepthUShort:
            image.write( 0, 0, width, height, "RGBA", Magick::ShortPixel, (ushort*)dstImg->getPixelData() );
            break;
        case eBitDepthFloat:
            image.write( 0, 0, width, height, "RGBA", Magick::FloatPixel, (float*)dstImg->getPixelData() );
            break;
        default:;
        }
    }
}

bool TexturePlugin::getRegionOfDefinition(const RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        throwSuiteStatusException(kOfxStatFailed);
        return false;
    }
    return GeneratorPlugin::getRegionOfDefinition(args.time, rod);
}

mDeclarePluginFactory(TexturePluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void TexturePluginFactory::describe(ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("Texture/Background generator node.");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    //desc.addSupportedBitDepth(eBitDepthUByte);
    //desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultipleClipPARs(kSupportsMultipleClipPARs);
    desc.setSupportsMultipleClipDepths(kSupportsMultipleClipDepths);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setTemporalClipAccess(false);
    desc.setRenderTwiceAlways(false);

    generatorDescribe(desc);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TexturePluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{   
    gHostIsNatron = (getImageEffectHostDescription()->isNatron);

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
            param->appendOption("Gradient/Linear");
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
            param->appendOption("Gradient Linear");
            param->appendOption("Loops 1");
            param->appendOption("Loops 2");
            param->appendOption("Loops 3");
        }
        param->setDefault(kParamEffectDefault);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamSeed);
        param->setLabel(kParamSeedLabel);
        param->setHint(kParamSeedHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 5000);
        param->setDefault(kParamSeedDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamFromColor);
        param->setLabel(kParamFromColorLabel);
        param->setHint(kParamFromColorHint);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamToColor);
        param->setLabel(kParamToColorLabel);
        param->setHint(kParamToColorHint);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);
        param->setLayoutHint(eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    generatorDescribeInContext(page, desc, *dstClip, eGeneratorExtentDefault, ePixelComponentRGBA, false,  context);
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TexturePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TexturePlugin(handle);
}

static TexturePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
