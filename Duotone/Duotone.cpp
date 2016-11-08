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

#include "OCLPlugin.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "Duotone"
#define kPluginGrouping "Color"
#define kPluginIdentifier "net.fxarena.opencl.Duotone"
#define kPluginDescription "Duotone color filter using OpenCL."
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamDarkColor "darkColor"
#define kParamDarkColorLabel "Dark Color"
#define kParamDarkColorHint "Dark Color."

#define kParamLightColor "lightColor"
#define kParamLightColorLabel "Light Color"
#define kParamLightColorHint "Light Color."

class DuotoneCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    DuotoneCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle, "", kPluginIdentifier)
        , _darkColor(0)
        , _lightColor(0)
    {
        _darkColor = fetchRGBParam(kParamDarkColor);
        _lightColor = fetchRGBParam(kParamLightColor);
        assert(_darkColor && _lightColor);
    }

    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) OVERRIDE FINAL
    {
        double dr, dg, db;
        double lr, lg, lb;
        _darkColor->getValueAtTime(args.time, dr, dg, db);
        _lightColor->getValueAtTime(args.time, lr, lg, lb);
        kernel.setArg(2, dr);
        kernel.setArg(3, dg);
        kernel.setArg(4, db);
        kernel.setArg(5, lr);
        kernel.setArg(6, lg);
        kernel.setArg(7, lb);
    }
private:
    RGBParam *_darkColor;
    RGBParam *_lightColor;
};

mDeclarePluginFactory(DuotoneCLPluginFactory, {}, {});

void DuotoneCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void DuotoneCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = DuotoneCLPlugin::describeInContextBegin(desc, context);
    {
        RGBParamDescriptor* param = desc.defineRGBParam(kParamDarkColor);
        param->setLabel(kParamDarkColorLabel);
        param->setHint(kParamDarkColorHint);
        param->setDefault(0.2345506, 0.0, 0.0);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        RGBParamDescriptor* param = desc.defineRGBParam(kParamLightColor);
        param->setLabel(kParamLightColorLabel);
        param->setHint(kParamLightColorHint);
        param->setDefault(1.0, 1.0, 0.0);
        param->setAnimates(true);
        param->setLayoutHint(eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    DuotoneCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
DuotoneCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new DuotoneCLPlugin(handle);
}

static DuotoneCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
