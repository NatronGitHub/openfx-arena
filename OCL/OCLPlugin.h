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

#ifndef OCLPlugin_h
#define OCLPlugin_h

#include "ofxsImageEffect.h"
#include "ofxsMacros.h"

// OpenCL helper functions, should probably just make my own functions and put them here, but for now this is "good enough"
#include "openCLUtilities.hpp"

#define kParamCLType "CLType"
#define kParamCLTypeLabel "Device Type"
#define kParamCLTypeHint "Select OpenCL device type. Default is to select the first avaiable device type."
#define kParamCLTypeDefault 2

#define kParamCLVendor "CLVendor"
#define kParamCLVendorLabel "Platform Vendor"
#define kParamCLVendorHint "Select OpenCL vendor type. Default is to select the first available vendor."
#define kParamCLVendorDefault 0

class OCLPluginHelperBase
    : public OFX::ImageEffect
{
public:

    OCLPluginHelperBase(OfxImageEffectHandle handle, const std::string &kernelSource);
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE;
    static OFX::PageParamDescriptor* describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context);
    static void describeInContextEnd(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context, OFX::PageParamDescriptor* page);
    void setupContext();

protected:
    OFX::Clip *_dstClip;
    OFX::Clip *_srcClip;
    cl::Context _context;
    cl::Program _program;
    OFX::ChoiceParam *_clType;
    OFX::ChoiceParam *_clVendor;
    std::string _source;
    int _renderscale;
};

template <int SupportsRenderScale>
class OCLPluginHelper
    : public OCLPluginHelperBase
{
public:

    OCLPluginHelper(OfxImageEffectHandle handle, const std::string &kernelSource)
        : OCLPluginHelperBase(handle, kernelSource)
    {
        _renderscale = SupportsRenderScale;
    }

    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) = 0;
    static OFX::PageParamDescriptor* describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context)
    {
        return OCLPluginHelperBase::describeInContextBegin(desc, context);
    }
};

template <int SupportsRenderScale>
void OCLPluginHelper<SupportsRenderScale>::render(const OFX::RenderArguments &args)
{
    // render scale
    if (!SupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get src clip
    if (!_srcClip) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(_srcClip);
    std::auto_ptr<const OFX::Image> srcImg(_srcClip->fetchImage(args.time));
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
        return;
    }

    // get dest clip
    if (!_dstClip) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(_dstClip);
    std::auto_ptr<OFX::Image> dstImg(_dstClip->fetchImage(args.time));
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

    // get image size
    int width = args.renderWindow.x2 - args.renderWindow.x1;
    int height = args.renderWindow.y2 - args.renderWindow.y1;

    // setup device
    // Currently we just take the first device available,
    // make it possible to select actual device in the future
    VECTOR_CLASS<cl::Device> devices = _context.getInfo<CL_CONTEXT_DEVICES>();

    // setup kernel
    // Notice we hardcode the main void as 'filter', this just simplify things
    cl::Kernel kernel(_program, "filter");

    // setup extra kernel args
    // Notice that 0/1 is reserved for input/output, don't override them!
    render(args, kernel);

    // setup and run queue
    cl::ImageFormat format(CL_RGBA, CL_FLOAT);
    cl::Image2D in(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, format, width, height, 0, (float*)srcImg->getPixelData());
    cl::Image2D out(_context, CL_MEM_WRITE_ONLY, format, width, height, 0, NULL);
    cl::CommandQueue queue = cl::CommandQueue(_context, devices[0]);
    cl::Event timer;
    cl::size_t<3> origin;
    cl::size_t<3> size;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    size[0] = width;
    size[1] = height;
    size[2] = 1;

    kernel.setArg(0, in);
    kernel.setArg(1, out);

    queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &timer);
    timer.wait();

    queue.enqueueReadImage(out, CL_TRUE, origin, size, 0, 0, (float*)dstImg->getPixelData());
    queue.finish();
}

template <int SupportsRenderScale>
bool OCLPluginHelper<SupportsRenderScale>::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!SupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }
    if (_srcClip && _srcClip->isConnected()) {
        rod = _srcClip->getRegionOfDefinition(args.time);
    } else {
        rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
    }
    return true;
}

#endif // OCLPlugin_h
