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

#include "ofxsMacros.h"
#include "ofxsImageEffect.h"
#include "ofxsTransform3x3.h"
#include "ofxsTransformInteractCustom.h"
#include "openCLUtilities.hpp"
#include <cmath>

#define kPluginName "SwirlCL"
#define kPluginGrouping "OCL"
#define kPluginIdentifier "fr.inria.openfx.SwirlCL"
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

#define kParamCLType "CLType"
#define kParamCLTypeLabel "Device"
#define kParamCLTypeHint "Switch between OpenCL on GPU, CPU, or all."
#define kParamCLTypeDefault 0

#define kParamCLVendor "CLVendor"
#define kParamCLVendorLabel "Vendor"
#define kParamCLVendorHint "Select OpenCL vendor. Currently any (select the 'best' alternative), NVIDIA, AMD or Intel."
#define kParamCLVendorDefault 0

#define kParamSwirlDefault 15

#define kParamKernel \
"#ifdef cl_khr_fp64\n" \
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n" \
"#elif defined(cl_amd_fp64)\n" \
"#pragma OPENCL EXTENSION cl_amd_fp64 : enable\n" \
"#endif\n" \
"const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;\n" \
"\n" \
"float clampRGB( float x ) {\n" \
"    return x > 1.f ? 1.f\n" \
"         : x < 0.f   ? 0.f\n" \
"         : x;\n" \
"}\n" \
"\n" \
"kernel void swirl (read_only image2d_t in, write_only image2d_t out, double centerW, double centerH, double amount, double radius, int interpolation) {\n" \
"\n" \
"    int2 d = get_image_dim(in);\n" \
"    int2 pos = (int2)(get_global_id(0),get_global_id(1));\n" \
"    int x = pos.x - centerW;\n" \
"    int y = pos.y - centerH;\n" \
"\n" \
"    float a = amount*exp(-(x*x+y*y)/(radius*radius));\n" \
"    float u = (cos(a)*x + sin(a)*y);\n" \
"    float v = (-sin(a)*x + cos(a)*y);\n" \
"\n" \
"    u += (float)centerW;\n" \
"    v += (float)centerH;\n" \
"\n" \
"    float4 fp = read_imagef(in,sampler,(int2)((int)u,(int)v));\n" \
"\n" \
"    // Interpolation\n" \
"    int2 p11 = (int2)(floor(u),floor(v));\n" \
"    float dx = u-(float)p11.x;\n" \
"    float dy = v-(float)p11.y;\n" \
"\n" \
"    float4 C[5];\n" \
"    float4 d0,d2,d3,a0,a1,a2,a3;\n" \
"\n" \
"    for (int i = 0; i < 4; i++) {\n" \
"        d0 = read_imagef(in,sampler,(int2)((int)u-1,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));\n" \
"        d2 = read_imagef(in,sampler,(int2)((int)u+1,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));\n" \
"        d3 = read_imagef(in,sampler,(int2)((int)u+2,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));\n" \
"        a0 = read_imagef(in,sampler,(int2)((int)u,  (int)v+i));\n" \
"        a1 =  -1.0f/3.f*d0 + d2 - 1.0f/6.f*d3;\n" \
"        a2 = 1.0f/2.f*d0 + 1.0f/2.f*d2;\n" \
"        a3 = -1.0f/6.f*d0 - 1.0f/2.f*d2 + 1.0f/6.f*d3;\n" \
"\n" \
"        C[i] = a0 + a1*dx + a2*dx*dx + a3*dx*dx*dx;\n" \
"    }\n" \
"\n" \
"    d0 = C[0]-C[1];\n" \
"    d2 = C[2]-C[1];\n" \
"    d3 = C[3]-C[1];\n" \
"    a0 = C[1];\n" \
"    a1 = -1.0f/3.f*d0 + d2 -1.0f/6.f*d3;\n" \
"    a2 = 1.0f/2.f*d0 + 1.0f/2.f*d2;\n" \
"    a3 = -1.0f/6.f*d0 - 1.0f/2.f*d2 + 1.0f/6.f*d3;\n" \
"    fp = (float4)(a0 + a1*dy + a2*dy*dy + a3*dy*dy*dy);\n" \
"    fp.x = clampRGB(fp.x);\n" \
"    fp.y = clampRGB(fp.y);\n" \
"    fp.z = clampRGB(fp.z);\n" \
"    fp.w = clampRGB(fp.w);\n" \
"\n" \
"    write_imagef(out,(int2)(pos.x,pos.y),fp);\n" \
"}"


using namespace OFX;

class SwirlCLPlugin : public ImageEffect
{
public:
    SwirlCLPlugin(OfxImageEffectHandle handle);
    virtual ~SwirlCLPlugin();
    virtual void render(const RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    void resetCenter(double time);
    void setupCL();
private:
    Clip *dstClip_;
    Clip *srcClip_;
    cl::Context context_;
    cl::Program program_;
    DoubleParam *amount_;
    Double2DParam *radius_;
    Double2DParam *position_;
    ChoiceParam *clType_;
    ChoiceParam *clVendor_;
    ChoiceParam *interpolation_;
};

SwirlCLPlugin::SwirlCLPlugin(OfxImageEffectHandle handle)
: ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
, amount_(0)
, radius_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == ePixelComponentRGBA);

    amount_ = fetchDoubleParam(kParamTransformRotateOld);
    radius_ = fetchDouble2DParam(kParamTransformScaleOld);
    position_ = fetchDouble2DParam(kParamTransformCenterOld);
    interpolation_ = fetchChoiceParam(kParamIP);

    clType_ = fetchChoiceParam(kParamCLType);
    clVendor_ = fetchChoiceParam(kParamCLVendor);

    assert(amount_ && radius_ && position_ && interpolation_ && clType_ && clVendor_);

    setupCL();
}

SwirlCLPlugin::~SwirlCLPlugin()
{

}

void SwirlCLPlugin::resetCenter(double time) {
    if (!dstClip_) {
        return;
    }
    OfxRectD rod = dstClip_->getRegionOfDefinition(time);
    if ( (rod.x1 <= kOfxFlagInfiniteMin) || (kOfxFlagInfiniteMax <= rod.x2) ||
         ( rod.y1 <= kOfxFlagInfiniteMin) || ( kOfxFlagInfiniteMax <= rod.y2) ) {
        return;
    }
    OfxPointD newCenter;
    newCenter.x = (rod.x1 + rod.x2) / 2;
    newCenter.y = (rod.y1 + rod.y2) / 2;
    if (position_) {
        position_->setValue(newCenter.x, newCenter.y);
    }
}

void SwirlCLPlugin::setupCL()
{
    int type, vendor;
    cl_device_type CLtype;
    cl_vendor CLvendor;
    clType_->getValue(type);
    clVendor_->getValue(vendor);

    switch(vendor) {
    case 1:
        CLvendor = VENDOR_NVIDIA;
        break;
    case 2:
        CLvendor = VENDOR_AMD;
        break;
    case 3:
        CLvendor = VENDOR_INTEL;
        break;
    default:
        CLvendor = VENDOR_ANY;
        break;
    }

    switch(type) {
    case 1:
        CLtype = CL_DEVICE_TYPE_GPU;
        break;
    case 2:
        CLtype = CL_DEVICE_TYPE_CPU;
        break;
    default:
        CLtype = CL_DEVICE_TYPE_ALL;
        break;
    }

    context_ = createCLContext(CLtype, CLvendor);
    program_ = buildProgramFromString(context_, kParamKernel);
}

void SwirlCLPlugin::render(const RenderArguments &args)
{
    // render scale
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get src clip
    if (!srcClip_) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(srcClip_);
    std::auto_ptr<const Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        if (srcImg->getRenderScale().x != args.renderScale.x ||
            srcImg->getRenderScale().y != args.renderScale.y ||
            srcImg->getField() != args.fieldToRender) {
            setPersistentMessage(Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
            throwSuiteStatusException(kOfxStatFailed);
            return;
        }
    } else {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get dest clip
    if (!dstClip_) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);
    std::auto_ptr<Image> dstImg(dstClip_->fetchImage(args.time));
    if (!dstImg.get()) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    if (dstImg->getRenderScale().x != args.renderScale.x ||
        dstImg->getRenderScale().y != args.renderScale.y ||
        dstImg->getField() != args.fieldToRender) {
        setPersistentMessage(Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get bit depth
    BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != eBitDepthFloat || (srcImg.get() && (dstBitDepth != srcImg->getPixelDepth()))) {
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get pixel component
    PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != ePixelComponentRGBA|| (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds?
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // get params
    double amount, radius, radiusX, radiusY, x, y;
    int interpolation;
    amount_->getValueAtTime(args.time, amount);
    radius_->getValueAtTime(args.time, radiusX, radiusY);
    position_->getValueAtTime(args.time, x, y);
    interpolation_->getValueAtTime(args.time, interpolation);

    radius = radiusX*100;
    if (amount != 0) {
        amount = amount/10;
    }

    // Position x y
    double ypos = y*args.renderScale.y;
    double xpos = x*args.renderScale.x;

    // setup kernel
    cl::Kernel kernel(program_, "swirl");
    // kernel arg 0 & 1 is reserved for input & output
    kernel.setArg(2, xpos);
    kernel.setArg(3, ypos);
    kernel.setArg(4, amount);
    kernel.setArg(5, std::floor(radius * args.renderScale.x + 0.5));
    kernel.setArg(6, interpolation);

    // get available devices
    VECTOR_CLASS<cl::Device> devices = context_.getInfo<CL_CONTEXT_DEVICES>();

    // render
    renderCL((float*)dstImg->getPixelData(), (float*)srcImg->getPixelData(), args.renderWindow.x2 - args.renderWindow.x1, args.renderWindow.y2 - args.renderWindow.y1, context_, devices[0], kernel);
}

void SwirlCLPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (paramName == kParamTransformResetCenterOld) {
        resetCenter(args.time);
    }
    else if (paramName == kParamCLType || paramName == kParamCLVendor) {
        setupCL();
    }

    clearPersistentMessage();
}

bool SwirlCLPlugin::getRegionOfDefinition(const RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        throwSuiteStatusException(kOfxStatFailed);
        return false;
    }
    if (srcClip_ && srcClip_->isConnected()) {
        rod = srcClip_->getRegionOfDefinition(args.time);
    } else {
        rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
    }
    return true;
}

mDeclarePluginFactory(SwirlCLPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void SwirlCLPluginFactory::describe(ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // add other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
    desc.setHostMaskingEnabled(kHostMasking);
    desc.setHostMixingEnabled(kHostMixing);

    Transform3x3Describe(desc, true);
    desc.setOverlayInteractDescriptor(new TransformOverlayDescriptorOldParams);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void SwirlCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // create param(s)
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    ofxsTransformDescribeParams(desc, page, NULL, /*isOpen=*/ true, /*oldParams=*/ true, /*noTranslate=*/ true, /*uniform=*/ true, /*rotateDefault*/ kParamSwirlDefault);
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamIP);
        param->setLabel(kParamIPLabel);
        param->setHint(kParamIPHint);
        param->setAnimates(false);
        param->setDefault(kParamIPDefault);
        param->setLayoutHint(eLayoutHintDivider);
        param->appendOption("Bilinear");
        param->appendOption("Bicubic");
        param->appendOption("None");
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamCLType);
        param->setLabel(kParamCLTypeLabel);
        param->setHint(kParamCLTypeHint);
        param->setAnimates(false);
        param->setDefault(kParamCLTypeDefault);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        param->appendOption("All");
        param->appendOption("GPU");
        param->appendOption("CPU");
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamCLVendor);
        param->setLabel(kParamCLVendorLabel);
        param->setHint(kParamCLVendorHint);
        param->setAnimates(false);
        param->setDefault(kParamCLVendorDefault);
        param->appendOption("Any");
        param->appendOption("NVIDIA");
        param->appendOption("AMD");
        param->appendOption("Intel");
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref ImageEffect class */
ImageEffect* SwirlCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new SwirlCLPlugin(handle);
}

static SwirlCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
