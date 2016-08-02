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

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "EdgeOCL"
#define kPluginGrouping "OpenCL"
#define kPluginIdentifier "net.fxarena.opencl.Edge"
#define kPluginDescription "OpenCL Edge Filter"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

const std::string kernelSource = \
"const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;\n"
"kernel void filter(read_only image2d_t input, write_only image2d_t output, int type) {\n"
"   const int2 p = {get_global_id(0), get_global_id(1)};\n"
"   float m[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };\n"
"   float2 t = {0.f, 0.f};\n"
"   for (int j = -1; j <= 1; j++) {\n"
"      for (int i = -1; i <= 1; i++) {\n"
"          float4 pix = read_imagef(input, sampler, (int2)(p.x+i, p.y+j));\n"
"          t.x += (pix.x*0.299f + pix.y*0.587f + pix.z*0.114f) * m[i+1][j+1];\n"
"          t.y += (pix.x*0.299f + pix.y*0.587f + pix.z*0.114f) * m[j+1][i+1];\n"
"      }\n"
"   }\n"
"   float o = sqrt(t.x*t.x + t.y*t.y);\n"
"   write_imagef(output, p, (float4)(o, o, o, 1.0f));\n"
"}";

class EdgeCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    EdgeCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle,kernelSource)
    {
    }

    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) OVERRIDE FINAL
    {
        kernel.setArg(2, 0);
    }
};

mDeclarePluginFactory(EdgeCLPluginFactory, {}, {});

void EdgeCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void EdgeCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = EdgeCLPlugin::describeInContextBegin(desc, context);
    EdgeCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
EdgeCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new EdgeCLPlugin(handle);
}

static EdgeCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
