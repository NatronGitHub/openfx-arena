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

#define kPluginName "CartoonOCL"
#define kPluginGrouping "OpenCL"
#define kPluginIdentifier "net.fxarena.opencl.Cartoon"
#define kPluginDescription "OpenCL Cartoon Filter"
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
"const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;\n"
"\n"
"float3 gray_internal(float4 color) {\n"
"  float y = dot(color.xyz, (float3)(0.2126f, 0.7152f, 0.0722f));\n"
"  return (float3)(y,y,y);\n"
"}\n"
"\n"
"__kernel void filter(__read_only image2d_t input, __write_only image2d_t output){\n"
"\n"
"   const int2 size = get_image_dim(input);\n"
"   int2 coord = (int2)(get_global_id(0),get_global_id(1));\n"
"   float4 color = read_imagef(input,sampler,convert_float2(coord));\n"
"   float dx = 1.0f / size.x;\n"
"   float dy = 1.0f / size.y;\n"
"\n"
"  float3 upperLeft   = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)(0.0f, -dy)));\n"
"  float3 upperCenter = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)(0.0f, -dy)));\n"
"  float3 upperRight  = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)( dx, -dy)));\n"
"  float3 left        = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)(-dx, 0.0f)));\n"
"  float3 center      = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)(0.0f, 0.0f)));\n"
"  float3 right       = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)( dx, 0.0f)));\n"
"  float3 lowerLeft   = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)(-dx,  dy)));\n"
"  float3 lowerCenter = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)(0.0f,  dy)));\n"
"  float3 lowerRight  = gray_internal(read_imagef(input,sampler, convert_float2(coord) + (float2)( dx,  dy)));\n"
"  \n"
"   float3 vertical  = upperLeft   * -1.0f\n"
"                 + upperCenter *  0.0f\n"
"                 + upperRight  *  1.0f\n"
"                 + left        * -2.0f\n"
"                 + center      *  0.0f\n"
"                 + right       *  2.0f\n"
"                 + lowerLeft   * -1.0f\n"
"                 + lowerCenter *  0.0f\n"
"                 + lowerRight  *  1.0f;\n"
"\n"
"  float3 horizontal = upperLeft   * -1.0f\n"
"                  + upperCenter * -2.0f\n"
"                  + upperRight  * -1.0f\n"
"                  + left        *  0.0f\n"
"                  + center      *  0.0f\n"
"                  + right       *  0.0f\n"
"                  + lowerLeft   *  1.0f\n"
"                  + lowerCenter *  2.0f\n"
"                  + lowerRight  *  1.0f;\n"
"\n"
"  float r = (vertical.x > 0 ? vertical.x : -vertical.x) + (horizontal.x > 0 ? horizontal.x : -horizontal.x);\n"
"  float g = (vertical.y > 0 ? vertical.x : -vertical.y) + (horizontal.y > 0 ? horizontal.y : -horizontal.y);\n"
"  float b = (vertical.z > 0 ? vertical.x : -vertical.z) + (horizontal.z > 0 ? horizontal.z : -horizontal.z);\n"
"  if (r > 1.0f) r = 1.0f;\n"
"  if (g > 1.0f) g = 1.0f;\n"
"  if (b > 1.0f) b = 1.0f;\n"
"  \n"
"  float4 edged = (float4)(color.xyz - (float3)(r, g, b), color.w);\n"
"  float arg = 1.0f;\n"
"\n"
"   write_imagef(output,coord,(float4)(mix(color.xyz, edged.xyz, arg), color.w));\n"
"}\n";

class CartoonCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    CartoonCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle,kernelSource)
    {
    }

    virtual void render(const OFX::RenderArguments &/*args*/, cl::Kernel /*kernel*/) OVERRIDE FINAL
    {
    }
};

mDeclarePluginFactory(CartoonCLPluginFactory, {}, {});

void CartoonCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void CartoonCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = CartoonCLPlugin::describeInContextBegin(desc, context);
    CartoonCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
CartoonCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new CartoonCLPlugin(handle);
}

static CartoonCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
