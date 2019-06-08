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

#include "OCLPlugin.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "CLFilter"
#define kPluginGrouping "Filter"
#define kPluginIdentifier "net.fxarena.opencl.CLFilter"
#define kPluginDescription "OpenCL filter meta node."
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamKernel "kernel"
#define kParamKernelLabel "Kernel"
#define kParamKernelHint "The kernel to build and use."
#define kParamKernelDefault \
    "const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;\n" \
    "void kernel filter(__read_only image2d_t input, __write_only image2d_t output)\n" \
    "{ /* basic image copy: */ \n"\
        "int2 pos = {get_global_id(0), get_global_id(1)};\n" \
        "float4 pixels = read_imagef(input, sampler, pos);\n" \
        "write_imagef(output, pos, pixels);\n" \
    "}\n"

class CLFilterCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    CLFilterCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle, "", "")
        , _kSource(NULL)
    {
        _kSource = fetchStringParam(kParamKernel);
        assert(_kSource);

        std::string source;
        _kSource->getValue(source);
        setupContext(false, source);
    }

    virtual void render(const OFX::RenderArguments &/*args*/, cl::Kernel /*kernel*/) OVERRIDE FINAL
    {
    }
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
private:
    OFX::StringParam *_kSource;
};

void CLFilterCLPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    std::string source;
    _kSource->getValueAtTime(args.time, source);

    if (paramName == kParamOCLDevice) {
        setupContext(true, source);
    }
    else if (paramName == kParamKernel) {
        setupContext(false, source);
    }

    clearPersistentMessage();
}

mDeclarePluginFactory(CLFilterCLPluginFactory, {}, {});

void CLFilterCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void CLFilterCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = CLFilterCLPlugin::describeInContextBegin(desc, context);
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamKernel);
        param->setLabel(kParamKernelLabel);
        param->setHint(kParamKernelHint);
        param->setStringType(eStringTypeMultiLine);
        param->setAnimates(true);
        param->setDefault(kParamKernelDefault);
        page->addChild(*param);
    }
    CLFilterCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
CLFilterCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new CLFilterCLPlugin(handle);
}

static CLFilterCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
