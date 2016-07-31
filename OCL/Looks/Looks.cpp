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

#define kPluginName "LooksOCL"
#define kPluginGrouping "OpenCL"
#define kPluginIdentifier "net.fxarena.opencl.Looks"
#define kPluginDescription "OpenCL Looks Filter"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamLook "preset"
#define kParamLookLabel "Preset"
#define kParamLookHint "Select color preset"
#define kParamLookDefault 0

const std::string kernelSource = \
"const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;\n"
"\n"
"void color_matrix_4x5(__read_only image2d_t input,__write_only image2d_t output,float * mask) {\n"
"   const int2 dim = get_image_dim(input);\n"
"   float2 coord = (float2)(get_global_id(0),get_global_id(1));\n"
"   float4 color = read_imagef(input,sampler,coord) * 255.0f;\n"
"   float4 rgba;\n"
"\n"
"   rgba.x = mask[0] * color.x + mask[1] * color.y + mask[2] * color.z + mask[3] * color.w + mask[4];\n"
"   rgba.y = mask[0 + 5] * color.x + mask[1 + 5] * color.y + mask[2 + 5] * color.z + mask[3 + 5] * color.w + mask[4 + 5];\n"
"   rgba.z = mask[0 + 5 * 2] * color.x + mask[1 + 5 * 2] * color.y + mask[2 + 5 * 2] * color.z + mask[3 + 5 * 2] * color.w + mask[4 + 5 * 2];\n"
"   rgba.w = mask[0 + 5 * 3] * color.x + mask[1 + 5 * 3] * color.y + mask[2 + 5 * 3] * color.z + mask[4 + 5 * 3] * color.w + mask[4 + 5 * 3];\n"
"   rgba = clamp(rgba,0.0f,255.0f);\n"
"   rgba /= 255.0f;\n"
"\n"
"   write_imagef(output,convert_int2(coord),rgba);\n"
"}\n"
"\n"
"kernel void filter(__read_only image2d_t input, __write_only image2d_t output, int preset) {\n"
"\n"
"   float polaroid[20] = {1.438f,-0.062f,-0.062f,0,0,-0.122f,1.378f,-0.122f,0,0,-0.016f,-0.016f,1.483f,0,0,0,0,0,1,0};\n"
"   float lomo[20] = {1.7f,0.1f,0.1f,0.0f,-73.1f,0.0f,1.7f,0.1f,0.0f,-73.1f,0.0f,0.1f,1.6f,0.0f,-73.1f,0.0f,0.0f,0.0f,1.0f,0.0f};\n"
"   float kodachrome[20] = {1.1285582396593525f,-0.3967382283601348f,-0.03992559172921793f,0,63.72958762196502f,-0.16404339962244616f,1.0835251566291304f,-0.05498805115633132f,0,24.732407896706203f,-0.16786010706155763f,-0.5603416277695248f,1.6014850761964943f,0,35.62982807460946f,0,0,0,1,0};\n"
"   float technicolor[20] = {1.9125277891456083f,-0.8545344976951645f,-0.09155508482755585f,0,11.793603434377337f,-0.3087833385928097f,1.7658908555458428f,-0.10601743074722245f,0,-70.35205161461398f,-0.231103377548616f,-0.7501899197440212f,1.847597816108189f,0,30.950940869491138f,0,0,0,1,0}; \n"
"   float vintage[20] = {0.6279345635605994f,0.3202183420819367f,-0.03965408211312453f,0.0f,9.3651285835294123f,0.02578397704808868f,0.6441188644374771f,0.03259127616149294f,0.0f,7.462829176470591f,0.0466055556782719f,-0.0851232987247891f,0.5241648018700465f,0.0f,5.159190588235296f,0.0f,0.0f,0.0f,1.0f,0.0f};\n"
"   float sepia[20] = {0.393f, 0.7689999f, 0.18899999f,0.0f,0.0f,0.349f, 0.6859999f, 0.16799999f,0.0f,0.0f,0.272f, 0.5339999f, 0.13099999f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f};\n"
"\n"
"   switch(preset) {\n"
"   case 0:\n"
"       color_matrix_4x5(input, output, polaroid);\n"
"       break;\n"
"   case 1:\n"
"       color_matrix_4x5(input, output, lomo);\n"
"       break;\n"
"   case 2:\n"
"       color_matrix_4x5(input, output, kodachrome);\n"
"       break;\n"
"   case 3:\n"
"       color_matrix_4x5(input, output, technicolor);\n"
"       break;\n"
"   case 4:\n"
"       color_matrix_4x5(input, output, vintage);\n"
"       break;\n"
"   case 5:\n"
"       color_matrix_4x5(input, output, sepia);\n"
"       break;\n"
"   }\n"
"}\n";

class LooksCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    LooksCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle,kernelSource)
        , _preset(0)
    {
        _preset = fetchChoiceParam(kParamLook);
        assert(_preset);
    }

    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) OVERRIDE FINAL
    {
        int preset = 0;
        _preset->getValueAtTime(args.time, preset);
        kernel.setArg(2, preset);
    }
private:
    ChoiceParam *_preset;
};

mDeclarePluginFactory(LooksCLPluginFactory, {}, {});

void LooksCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void LooksCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = LooksCLPlugin::describeInContextBegin(desc, context);
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamLook);
        param->setLabel(kParamLookLabel);
        param->setHint(kParamLookHint);
        param->setAnimates(false);
        param->setDefault(kParamLookDefault);
        param->setLayoutHint(eLayoutHintDivider);
        param->appendOption("Polaroid");
        param->appendOption("Lomo");
        param->appendOption("Kodachrome");
        param->appendOption("Technicolor");
        param->appendOption("Vintage");
        param->appendOption("Sepia");
        if (page) {
            page->addChild(*param);
        }
    }
    LooksCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
LooksCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new LooksCLPlugin(handle);
}

static LooksCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
