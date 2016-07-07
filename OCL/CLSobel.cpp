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
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include "openCLUtilities.hpp"

#define kPluginName "CLSobel"
#define kPluginGrouping "Extra/Filter"
#define kPluginIdentifier "fr.inria.openfx.CLSobel"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

#define kParamAmount "amount"
#define kParamAmountLabel "Amount"
#define kParamAmountHint "Edge Amount"
#define kParamAmountDefault -2

using namespace OFX;

class CLSobelPlugin : public OFX::ImageEffect
{
public:
    CLSobelPlugin(OfxImageEffectHandle handle);
    virtual ~CLSobelPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    cl::Context context_;
    cl::Program program_;
    OFX::DoubleParam *amount_;
};

CLSobelPlugin::CLSobelPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    amount_ = fetchDoubleParam(kParamAmount);
    assert(amount_);
    
    // TODO: move to own function and support selecting device etc
    context_ = createCLContext();
    std::string source =
        "const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;"
        "kernel void sobel(read_only image2d_t input, write_only image2d_t output, double amount) {"
        "int x = get_global_id(0);"
        "int y = get_global_id(1);"
        "float matrix[3][3] = {{-1, amount, -1},{0, 0, 0},{1, 2, 1}};"
        "float o_x = 0.f;"
        "float o_y = 0.f;"
        "for (int j = -1; j <= 1; j++) {"
        "   for (int i = -1; i <= 1; i++) {"
        "       float4 pix = read_imagef(input, sampler, (int2)(x+i, y+j));"
        "       o_x += (pix.x*0.299f + pix.y*0.587f + pix.z*0.114f) * matrix[i+1][j+1];"
        "       o_y += (pix.x*0.299f + pix.y*0.587f + pix.z*0.114f) * matrix[j+1][i+1];"
        "   }"
        "}"
        "float o = sqrt(o_x*o_x + o_y*o_y);"
        "write_imagef(output, (int2)(x, y), (float4)(o,o,o,1));}";
    program_ = buildProgramFromString(context_, source);
}

CLSobelPlugin::~CLSobelPlugin()
{
}

void CLSobelPlugin::render(const OFX::RenderArguments &args)
{
    // render scale
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get src clip
    if (!srcClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(srcClip_);
    std::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        if (srcImg->getRenderScale().x != args.renderScale.x ||
            srcImg->getRenderScale().y != args.renderScale.y ||
            srcImg->getField() != args.fieldToRender) {
            setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
            OFX::throwSuiteStatusException(kOfxStatFailed);
            return;
        }
    } else {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // get dest clip
    if (!dstClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);
    std::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    if (dstImg->getRenderScale().x != args.renderScale.x ||
        dstImg->getRenderScale().y != args.renderScale.y ||
        dstImg->getField() != args.fieldToRender) {
        setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get bit depth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && (dstBitDepth != srcImg->getPixelDepth()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get pixel component
    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != OFX::ePixelComponentRGBA|| (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds?
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    int width = args.renderWindow.x2 - args.renderWindow.x1;
    int height = args.renderWindow.y2 - args.renderWindow.y1;

    double amount = 0.0;
    amount_->getValueAtTime(args.time, amount);

    VECTOR_CLASS<cl::Device> devices = context_.getInfo<CL_CONTEXT_DEVICES>();
    cl::CommandQueue queue = cl::CommandQueue(context_, devices[0]);

    cl::size_t<3> origin;
    cl::size_t<3> size;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    size[0] = width;
    size[1] = height;
    size[2] = 1;

    cl::ImageFormat format(CL_RGBA, CL_FLOAT);
    cl::Image2D in(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, format, width, height, 0, (void*)srcImg->getPixelData());
    cl::Image2D out(context_, CL_MEM_WRITE_ONLY, format, width, height, 0, NULL);

    cl::Kernel kernel = cl::Kernel(program_, "sobel");
    kernel.setArg(0, in);
    kernel.setArg(1, out);
    kernel.setArg(2, amount);

    queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height), cl::NullRange);
    queue.enqueueReadImage(out, CL_TRUE, origin, size, 0, 0, (float*)dstImg->getPixelData());
    queue.finish();
}

bool CLSobelPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
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

mDeclarePluginFactory(CLSobelPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void CLSobelPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("OpenCL Sobel Filter.");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
    desc.setHostMaskingEnabled(true);
    desc.setHostMixingEnabled(true);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void CLSobelPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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

    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamAmount);
        param->setLabel(kParamAmountLabel);
        param->setHint(kParamAmountHint);
        param->setRange(-4, 0);
        param->setDisplayRange(-4, 0);
        param->setDefault(kParamAmountDefault);
        page->addChild(*param);
    }

}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* CLSobelPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new CLSobelPlugin(handle);
}

static CLSobelPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
