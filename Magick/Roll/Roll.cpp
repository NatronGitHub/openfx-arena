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

#define kPluginName "RollOFX"
#define kPluginGrouping "Extra/Transform"
#define kPluginIdentifier "net.fxarena.openfx.Roll"
#define kPluginDescription "Roll effect using ImageMagick."
#define kPluginVersionMajor 2
#define kPluginVersionMinor 9

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamRollX "x"
#define kParamRollXLabel "X"
#define kParamRollXHint "Adjust roll X"
#define kParamRollXDefault 0

#define kParamRollY "y"
#define kParamRollYLabel "Y"
#define kParamRollYHint "Adjust roll Y"
#define kParamRollYDefault 0

class RollPlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    RollPlugin(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
        , _x(0)
        , _y(0)
    {
        _x = fetchDoubleParam(kParamRollX);
        _y = fetchDoubleParam(kParamRollY);
        assert(_x && _y);
    }

    virtual void render(const OFX::RenderArguments &args, Magick::Image &image) OVERRIDE FINAL
    {
        double x,y;
        _x->getValueAtTime(args.time, x);
        _y->getValueAtTime(args.time, y);
        image.roll(std::floor(x * args.renderScale.x + 0.5), std::floor(y * args.renderScale.x + 0.5));
    }
private:
    DoubleParam *_x;
    DoubleParam *_y;
};

mDeclarePluginFactory(RollPluginFactory, {}, {});

void RollPluginFactory::describe(ImageEffectDescriptor &desc)
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

void RollPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = RollPlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamRollX);
        param->setLabel(kParamRollXLabel);
        param->setHint(kParamRollXHint);
        param->setRange(-100000, 100000);
        param->setDisplayRange(-2000, 2000);
        param->setDefault(kParamRollXDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamRollY);
        param->setLabel(kParamRollYLabel);
        param->setHint(kParamRollYHint);
        param->setRange(-100000, 100000);
        param->setDisplayRange(-2000, 2000);
        param->setDefault(kParamRollYDefault);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    RollPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
RollPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new RollPlugin(handle);
}

static RollPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
