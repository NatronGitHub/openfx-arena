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
#include "ofxsTransform3x3.h"
#include "ofxsTransformInteractCustom.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "SwirlOCL"
#define kPluginGrouping "OpenCL"
#define kPluginIdentifier "net.fxarena.opencl.Swirl"
#define kPluginDescription "OpenCL Swirl Filter"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamIP "interpolation"
#define kParamIPLabel "Interpolation"
#define kParamIPHint "Interpolation"
#define kParamIPDefault 0

#define kParamSwirlDefault 15

const std::string kernelSource = \
"#ifdef cl_khr_fp64\n"
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n"
"#elif defined(cl_amd_fp64)\n"
"#pragma OPENCL EXTENSION cl_amd_fp64 : enable\n"
"#endif\n"
"const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;\n"
"\n"
"float clampRGB( float x ) {\n"
"    return x > 1.f ? 1.f\n"
"         : x < 0.f   ? 0.f\n"
"         : x;\n"
"}\n"
"\n"
"kernel void filter (read_only image2d_t in, write_only image2d_t out, double centerW, double centerH, double amount, double radius) {\n"
"\n"
"    int2 d = get_image_dim(in);\n"
"    int2 pos = (int2)(get_global_id(0),get_global_id(1));\n"
"    int x = pos.x - centerW;\n"
"    int y = pos.y - centerH;\n"
"\n"
"    float a = amount*exp(-(x*x+y*y)/(radius*radius));\n"
"    float u = (cos(a)*x + sin(a)*y);\n"
"    float v = (-sin(a)*x + cos(a)*y);\n"
"\n"
"    u += (float)centerW;\n"
"    v += (float)centerH;\n"
"\n"
"    float4 fp = read_imagef(in,sampler,(int2)((int)u,(int)v));\n"
"\n"
"    // Interpolation\n"
"    int2 p11 = (int2)(floor(u),floor(v));\n"
"    float dx = u-(float)p11.x;\n"
"    float dy = v-(float)p11.y;\n"
"\n"
"    float4 C[5];\n"
"    float4 d0,d2,d3,a0,a1,a2,a3;\n"
"\n"
"    for (int i = 0; i < 4; i++) {\n"
"        d0 = read_imagef(in,sampler,(int2)((int)u-1,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));\n"
"        d2 = read_imagef(in,sampler,(int2)((int)u+1,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));\n"
"        d3 = read_imagef(in,sampler,(int2)((int)u+2,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));\n"
"        a0 = read_imagef(in,sampler,(int2)((int)u,  (int)v+i));\n"
"        a1 =  -1.0f/3.f*d0 + d2 - 1.0f/6.f*d3;\n"
"        a2 = 1.0f/2.f*d0 + 1.0f/2.f*d2;\n"
"        a3 = -1.0f/6.f*d0 - 1.0f/2.f*d2 + 1.0f/6.f*d3;\n"
"\n"
"        C[i] = a0 + a1*dx + a2*dx*dx + a3*dx*dx*dx;\n"
"    }\n"
"\n"
"    d0 = C[0]-C[1];\n"
"    d2 = C[2]-C[1];\n"
"    d3 = C[3]-C[1];\n"
"    a0 = C[1];\n"
"    a1 = -1.0f/3.f*d0 + d2 -1.0f/6.f*d3;\n"
"    a2 = 1.0f/2.f*d0 + 1.0f/2.f*d2;\n"
"    a3 = -1.0f/6.f*d0 - 1.0f/2.f*d2 + 1.0f/6.f*d3;\n"
"    fp = (float4)(a0 + a1*dy + a2*dy*dy + a3*dy*dy*dy);\n"
"    fp.x = clampRGB(fp.x);\n"
"    fp.y = clampRGB(fp.y);\n"
"    fp.z = clampRGB(fp.z);\n"
"    fp.w = clampRGB(fp.w);\n"
"\n"
"    write_imagef(out,(int2)(pos.x,pos.y),fp);\n"
"}";

class SwirlCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    SwirlCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle,kernelSource)
        , _position(0)
        , _radius(0)
        , _strength(0)
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
        kernel.setArg(4, s);
        kernel.setArg(5, r);
    }
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    void resetCenter(double time);
private:
    Double2DParam *_position;
    Double2DParam *_radius;
    DoubleParam *_strength;
};

void SwirlCLPlugin::resetCenter(double time) {
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

void SwirlCLPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (paramName == kParamOCLDevice) {
        setupContext();
    }
    else if (paramName == kParamTransformResetCenterOld) {
        resetCenter(args.time);
    }

    clearPersistentMessage();
}

mDeclarePluginFactory(SwirlCLPluginFactory, {}, {});

void SwirlCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void SwirlCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = SwirlCLPlugin::describeInContextBegin(desc, context);
    ofxsTransformDescribeParams(desc, page, NULL, /*isOpen=*/ true, /*oldParams=*/ true, /*noTranslate=*/ true, /*uniform=*/ true, /*rotateDefault*/ kParamSwirlDefault);
    SwirlCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
SwirlCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new SwirlCLPlugin(handle);
}

static SwirlCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
