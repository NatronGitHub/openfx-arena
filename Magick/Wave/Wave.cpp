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

#include "MagickPlugin.h"
#include <cmath>

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "WaveOFX"
#define kPluginGrouping "Extra/Distort"
#define kPluginIdentifier "net.fxarena.openfx.Wave"
#define kPluginDescription "Wave effect using ImageMagick."
// Version 4 crops the result of WaveImage so that it is aligned with the input (see WavePlugin::render),
// and unpremultiplies the source before ImageMagick (see PackedImage::PackedSrc).
#define kPluginVersionMajor 4
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
    // fixed is false for version 2, which fed premultiplied pixels to ImageMagick and
    // didn't align the output: version 4 fixes both
    WavePlugin(OfxImageEffectHandle handle, bool fixed)
        : MagickPluginHelper<kSupportsRenderScale>(handle, fixed)
        , _alignOutput(fixed)
        , _amp(NULL)
        , _length(NULL)
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
        const size_t rows = image.rows();
        image.wave(std::floor(waveAmp * args.renderScale.x + 0.5),std::floor(waveLength * args.renderScale.x + 0.5));
        if (_alignOutput && image.rows() > rows) {
            // WaveImage returns an image that is 2*|amplitude| rows taller, with the input centered
            // vertically. Version 2 kept the first rows, which shifted the picture by the amplitude
            // and lost rows at the top. Keep the rows that are aligned with the input instead.
            const ssize_t yOffset = (ssize_t)(image.rows() - rows) / 2;
            image.crop(Magick::Geometry(image.columns(), rows, 0, yOffset));
            image.page(Magick::Geometry(0, 0, 0, 0));
        }
    }
private:
    bool _alignOutput;
    DoubleParam *_amp;
    DoubleParam *_length;
};

mDeclarePluginFactoryVersioned(WavePluginFactory, {}, {});

template<unsigned int majorVersion>
void WavePluginFactory<majorVersion>::describe(ImageEffectDescriptor &desc)
{
    if (majorVersion < kPluginVersionMajor) {
        desc.setIsDeprecated(true);
    }
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

template<unsigned int majorVersion>
void WavePluginFactory<majorVersion>::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
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

template<unsigned int majorVersion>
OFX::ImageEffect*
WavePluginFactory<majorVersion>::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new WavePlugin(handle, majorVersion >= 4);
}

static WavePluginFactory<kPluginVersionMajor> p(kPluginIdentifier, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
// Version 2 is kept unchanged so that existing projects render as before.
static WavePluginFactory<2> p2(kPluginIdentifier, 9);
mRegisterPluginFactoryInstance(p2)

OFXS_NAMESPACE_ANONYMOUS_EXIT
