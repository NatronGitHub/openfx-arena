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

#define kPluginName "SharpenOCL"
#define kPluginGrouping "Filter"
#define kPluginIdentifier "net.fxarena.opencl.Sharpen"
#define kPluginDescription "Sharpen Filter using OpenCL."
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamFactor "factor"
#define kParamFactorLabel "Factor"
#define kParamFactorHint "Adjust the factor."
#define kParamFactorDefault 1.0

class SharpenCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    SharpenCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle, "", kPluginIdentifier)
        , _factor(0)
    {
        _factor = fetchDoubleParam(kParamFactor);
        assert(_factor);
    }

    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) OVERRIDE FINAL
    {
        double factor = 0.0;
        _factor->getValueAtTime(args.time, factor);
        kernel.setArg(2, factor);
    }
private:
    DoubleParam *_factor;
};

mDeclarePluginFactory(SharpenCLPluginFactory, {}, {});

void SharpenCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void SharpenCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = SharpenCLPlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamFactor);
        param->setLabel(kParamFactorLabel);
        param->setHint(kParamFactorHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamFactorDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    SharpenCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
SharpenCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new SharpenCLPlugin(handle);
}

static SharpenCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
