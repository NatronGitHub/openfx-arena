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
// 2.0: the centre is relative to the render window origin, so it is right when
// the RoD does not start at (0,0). 1.0 is kept, unchanged, for old projects.
#define kPluginVersionMajor 2
#define kPluginVersionMinor 0
#define kPluginVersionMajorV1 1
#define kPluginVersionMinorV1 0

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
    BulgeCLPlugin(OfxImageEffectHandle handle, bool centerFromRenderWindow)
        : OCLPluginHelper<kSupportsRenderScale>(handle, "", kPluginIdentifier)
        , _position(NULL)
        , _radius(NULL)
        , _strength(NULL)
        , _centerFromRenderWindow(centerFromRenderWindow)
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
        if (_centerFromRenderWindow) {
            // the kernel works in coordinates relative to the render window origin
            // (v1 ignored it, which is wrong when the RoD does not start at (0,0))
            ypos -= args.renderWindow.y1;
            xpos -= args.renderWindow.x1;
        }
        r = (rX*100)*args.renderScale.x;
        if (s!=0) {
            s = s/10;
        }
        // the kernel takes float args: double needs cl_khr_fp64, which many devices lack
        setKernelArg(kernel, 2, (cl_float)xpos);
        setKernelArg(kernel, 3, (cl_float)ypos);
        setKernelArg(kernel, 4, (cl_float)r);
        setKernelArg(kernel, 5, (cl_float)s);
    }
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    void resetCenter(double time);
private:
    Double2DParam *_position;
    Double2DParam *_radius;
    DoubleParam *_strength;
    bool _centerFromRenderWindow; // true from v2
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

    // clear first: setupContext() may set a new error message
    clearPersistentMessage();
    if (paramName == kParamOCLDevice) {
        setupContext(true, "");
    }
    else if (paramName == kParamTransformResetCenterOld) {
        resetCenter(args.time);
    }
}

mDeclarePluginFactoryVersioned(BulgeCLPluginFactory, {}, {});

template<unsigned int majorVersion>
void
BulgeCLPluginFactory<majorVersion>::describe(ImageEffectDescriptor &desc)
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
    // OFX 1.5: V2 (DrawSuite) overlay on hosts that have it, also keeping the OpenGL (V1) one;
    // overlays don't change what renders, so both major versions get it
    ofxsSetOverlayInteractDescriptorV2(desc, new TransformCustomOverlayDescriptorOldParamsV2);
}

template<unsigned int majorVersion>
void
BulgeCLPluginFactory<majorVersion>::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = BulgeCLPlugin::describeInContextBegin(desc, context);
    ofxsTransformDescribeParams(desc, page, NULL, /*isOpen=*/ true, /*oldParams=*/ true, /*noTranslate=*/ true, /*uniform=*/ true, /*rotateDefault*/ kParamStrengthDefault);
    BulgeCLPlugin::describeInContextEnd(desc, context, page);
}

template<unsigned int majorVersion>
OFX::ImageEffect*
BulgeCLPluginFactory<majorVersion>::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new BulgeCLPlugin(handle, /*centerFromRenderWindow=*/ majorVersion >= 2);
}

static BulgeCLPluginFactory<kPluginVersionMajor> p(kPluginIdentifier, kPluginVersionMinor);
static BulgeCLPluginFactory<kPluginVersionMajorV1> p1(kPluginIdentifier, kPluginVersionMinorV1);
mRegisterPluginFactoryInstance(p)
mRegisterPluginFactoryInstance(p1)

OFXS_NAMESPACE_ANONYMOUS_EXIT
