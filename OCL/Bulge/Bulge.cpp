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
#include "ofxsTransform3x3.h"
#include "ofxsTransformInteractCustom.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "Bulge"
#define kPluginGrouping "Transform"
#define kPluginIdentifier "net.fxarena.opencl.Bulge"
#define kPluginDescription "Bulge (implode/explode) transform effect using OpenCL."
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamStrengthDefault 4.0

class BulgeCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    BulgeCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle, "", kPluginIdentifier)
        , _position(NULL)
        , _radius(NULL)
        , _strength(NULL)
    {
        _position = fetchDouble2DParam(kParamTransformCenterOld);
        _radius = fetchDouble2DParam(kParamTransformScaleOld);
        _strength = fetchDoubleParam(kParamTransformRotateOld);
        assert(_position && _radius && _strength);
    }

    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) OVERRIDE FINAL
    {
        double x, y, r, rX, rY, s;
        _position->getValueAtTime(args.time, x, y);
        _radius->getValueAtTime(args.time, rX, rY);
        _strength->getValueAtTime(args.time, s);
        double ypos = y*args.renderScale.y;
        double xpos = x*args.renderScale.x;
        r = (rX*100)*args.renderScale.x;
        if (s!=0) {
            s = s/10;
        }
        kernel.setArg(2, xpos);
        kernel.setArg(3, ypos);
        kernel.setArg(4, r);
        kernel.setArg(5, s);
    }
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    void resetCenter(double time);
private:
    Double2DParam *_position;
    Double2DParam *_radius;
    DoubleParam *_strength;
};

void BulgeCLPlugin::resetCenter(double time) {
    if (!_dstClip) {
        return;
    }
    OfxRectD rod = _dstClip->getRegionOfDefinition(time);
    if ( (rod.x1 <= kOfxFlagInfiniteMin) || (kOfxFlagInfiniteMax <= rod.x2) ||
         ( rod.y1 <= kOfxFlagInfiniteMin) || ( kOfxFlagInfiniteMax <= rod.y2) ) {
        return;
    }
    OfxPointD newCenter;
    newCenter.x = (rod.x1 + rod.x2) / 2;
    newCenter.y = (rod.y1 + rod.y2) / 2;
    if (_position) {
        _position->setValue(newCenter.x, newCenter.y);
    }
}

void BulgeCLPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (paramName == kParamOCLDevice) {
        setupContext(true, "");
    }
    else if (paramName == kParamTransformResetCenterOld) {
        resetCenter(args.time);
    }

    clearPersistentMessage();
}

mDeclarePluginFactory(BulgeCLPluginFactory, {}, {});

void BulgeCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

    Transform3x3Describe(desc, true);
    desc.setOverlayInteractDescriptor(new TransformOverlayDescriptorOldParams);
}

void BulgeCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = BulgeCLPlugin::describeInContextBegin(desc, context);
    ofxsTransformDescribeParams(desc, page, NULL, /*isOpen=*/ true, /*oldParams=*/ true, /*noTranslate=*/ true, /*uniform=*/ true, /*rotateDefault*/ kParamStrengthDefault);
    BulgeCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
BulgeCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new BulgeCLPlugin(handle);
}

static BulgeCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
