/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
 * Copyright (C) 2015, 2016 FxArena DA
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

#include "OCLPlugin.h"
#include <cmath>

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "RippleOCL"
#define kPluginGrouping "OpenCL"
#define kPluginIdentifier "net.fxarena.opencl.Ripple"
#define kPluginDescription "OpenCL Ripple Filter"
#define kPluginVersionMajor 1
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
#define kParamWaveAmpHint "Adjust amplitude"
#define kParamWaveAmpDefault 25

#define kParamWaveLength "length"
#define kParamWaveLengthLabel "Length"
#define kParamWaveLengthHint "Adjust length"
#define kParamWaveLengthDefault 150

const std::string kernelSource = \
"const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;\n"
"int2 effect(float x, float y, float a, float w) {\n"
"        float m = sqrt(x*x + y*y);\n"
"        float s = sin(w*m);\n"
"        float x1 = a*y*m*s;\n"
"        float y1 = -a*x*m*s;\n"
"        return (int2)(x1, y1);\n"
"}\n"
"int2 displacement(int xi, int yi, int width, int height, double waveAmp, double waveLength) {\n"
"        float aspect = ((float) height) / ((float) width);\n"
"        float x = ((float) xi) / ((float) width), y = ((float) yi) / ((float) height);\n"
"        return effect(x - 0.5, aspect*(y - 0.5 + 0.35), waveAmp, waveLength)\n"
"             - effect(x - 0.5, aspect*(y - 0.5 - 0.35), waveAmp, waveLength)\n"
"             + effect(x - 0.5, aspect*(y), 2, 5000);\n"
"}\n"
"__kernel void filter(read_only image2d_t inputImage, write_only image2d_t outputImage, double waveAmp, double waveLength) {\n"
"   int2 dimensions = get_image_dim(inputImage);\n"
"   int width = dimensions.x, height = dimensions.y;\n"
"   int channelDataType = get_image_channel_data_type(inputImage);\n"
"   int channelOrder = get_image_channel_order(inputImage);\n"
"   int x = get_global_id(0), y = get_global_id(1);\n"
"   int2 coordinates = (int2)(x, y);\n"
"   int2 disp = displacement(x, y, width, height, waveAmp, waveLength);\n"
"   int2 fromCoordinates = coordinates + disp;\n"
"   float4 pixel = read_imagef(inputImage, sampler, fromCoordinates);\n"
"   float4 transformedPixel = pixel;\n"
"   write_imagef(outputImage, coordinates, transformedPixel);\n"
"}";

class RippleCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    RippleCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle,kernelSource)
        , _waveAmp(0)
        , _waveLength(0)
    {
        _waveAmp = fetchDoubleParam(kParamWaveAmp);
        _waveLength = fetchDoubleParam(kParamWaveLength);
        assert(_waveAmp && _waveLength);
    }

    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) OVERRIDE FINAL
    {
        double waveAmp, waveLength;
        _waveAmp->getValueAtTime(args.time, waveAmp);
        _waveLength->getValueAtTime(args.time, waveLength);
        kernel.setArg(2, std::floor(waveAmp * args.renderScale.x + 0.5));
        kernel.setArg(3, std::floor(waveLength * args.renderScale.x + 0.5));
    }
private:
    DoubleParam *_waveAmp;
    DoubleParam *_waveLength;
};

mDeclarePluginFactory(RippleCLPluginFactory, {}, {});

void RippleCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void RippleCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = RippleCLPlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWaveAmp);
        param->setLabel(kParamWaveAmpLabel);
        param->setHint(kParamWaveAmpHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamWaveAmpDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamWaveLength);
        param->setLabel(kParamWaveLengthLabel);
        param->setHint(kParamWaveLengthHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamWaveLengthDefault);
        param->setLayoutHint(eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    RippleCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
RippleCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new RippleCLPlugin(handle);
}

static RippleCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
