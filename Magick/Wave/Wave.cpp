/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * openfx-arena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
*/

#include "MagickPlugin.h"
#include <cmath>

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "WaveOFX"
#define kPluginGrouping "Extra/Distort"
#define kPluginIdentifier "net.fxarena.openfx.Wave"
#define kPluginDescription "Wave effect using ImageMagick."
#define kPluginVersionMajor 3
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamWaveAmp "amp"
#define kParamWaveAmpLabel "Amplitude"
#define kParamWaveAmpHint "Adjust wave amplitude"
#define kParamWaveAmpDefault 25

#define kParamWaveLength "length"
#define kParamWaveLengthLabel "Length"
#define kParamWaveLengthHint "Adjust wave length"
#define kParamWaveLengthDefault 150

class WavePlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    WavePlugin(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
        , _amp(0)
        , _length(0)
    {
        _amp = fetchDoubleParam(kParamWaveAmp);
        _length = fetchDoubleParam(kParamWaveLength);
        assert(_amp && _length);
    }

    virtual void render(const OFX::RenderArguments &args, Magick::Image &image) OVERRIDE FINAL
    {
        double waveAmp, waveLength;
        _amp->getValueAtTime(args.time, waveAmp);
        _length->getValueAtTime(args.time, waveLength);
        image.backgroundColor(Magick::Color("rgba(0,0,0,0)"));
        image.wave(std::floor(waveAmp * args.renderScale.x + 0.5),std::floor(waveLength * args.renderScale.x + 0.5));
    }
private:
    DoubleParam *_amp;
    DoubleParam *_length;
};

mDeclarePluginFactory(WavePluginFactory, {}, {});

void WavePluginFactory::describe(ImageEffectDescriptor &desc)
{
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);
    desc.addSupportedBitDepth(eBitDepthFloat);
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
    desc.setHostMaskingEnabled(kHostMasking);
    desc.setHostMixingEnabled(kHostMixing);
}

void WavePluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = WavePlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWaveAmp);
        param->setLabel(kParamWaveAmpLabel);
        param->setHint(kParamWaveAmpHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamWaveAmpDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWaveLength);
        param->setLabel(kParamWaveLengthLabel);
        param->setHint(kParamWaveLengthHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamWaveLengthDefault);
        page->addChild(*param);
    }
    WavePlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
WavePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new WavePlugin(handle);
}

static WavePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
