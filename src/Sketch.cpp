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

#define kPluginName "SketchOFX"
#define kPluginGrouping "Extra/Filter"
#define kPluginIdentifier "net.fxarena.openfx.Sketch"
#define kPluginDescription "Sketch effect node."
#define kPluginVersionMajor 3
#define kPluginVersionMinor 0

#define kParamRadius "radius"
#define kParamRadiusLabel "Radius"
#define kParamRadiusHint "Adjust radius"
#define kParamRadiusDefault 1

#define kParamSigma "sigma"
#define kParamSigmaLabel "Sigma"
#define kParamSigmaHint "Adjust sigma"
#define kParamSigmaDefault 0

#define kParamAngle "angle"
#define kParamAngleLabel "Angle"
#define kParamAngleHint "Adjust angle"
#define kParamAngleDefault 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

class SketchPlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    SketchPlugin(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
        , _radius(NULL)
        , _sigma(NULL)
        , _angle(NULL)
    {
        _radius = fetchDoubleParam(kParamRadius);
        _sigma = fetchDoubleParam(kParamSigma);
        _angle = fetchDoubleParam(kParamAngle);

        assert(_radius && _sigma && _angle);
    }

    virtual void render(const RenderArguments &args,
                        Magick::Image &image) OVERRIDE FINAL
    {
        double radius, sigma, angle;
        _radius->getValueAtTime(args.time, radius);
        _sigma->getValueAtTime(args.time, sigma);
        _angle->getValueAtTime(args.time, angle);

        image.sketch(std::floor(radius * args.renderScale.x + 0.5),
                     std::floor(sigma * args.renderScale.x + 0.5),
                     angle);
    }

private:
    DoubleParam *_radius;
    DoubleParam *_sigma;
    DoubleParam *_angle;
};

mDeclarePluginFactory(SketchPluginFactory, {}, {});

void SketchPluginFactory::describe(ImageEffectDescriptor &desc)
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

void SketchPluginFactory::describeInContext(ImageEffectDescriptor &desc,
                                            ContextEnum context)
{
    PageParamDescriptor *page = SketchPlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamRadius);
        param->setLabel(kParamRadiusLabel);
        param->setHint(kParamRadiusHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 50);
        param->setDefault(kParamRadiusDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSigma);
        param->setLabel(kParamSigmaLabel);
        param->setHint(kParamSigmaHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 50);
        param->setDefault(kParamSigmaDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamAngle);
        param->setLabel(kParamAngleLabel);
        param->setHint(kParamAngleHint);
        param->setRange(-360, 360);
        param->setDisplayRange(-360, 360);
        param->setDefault(kParamAngleDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    SketchPlugin::describeInContextEnd(desc, context, page);
}

ImageEffect* SketchPluginFactory::createInstance(OfxImageEffectHandle handle,
                                                 ContextEnum /*context*/)
{
    return new SketchPlugin(handle);
}

static SketchPluginFactory p(kPluginIdentifier,
                             kPluginVersionMajor,
                             kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
