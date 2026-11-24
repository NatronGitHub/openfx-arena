/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
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

#ifndef OCLPlugin_h
#define OCLPlugin_h

#include "ofxsImageEffect.h"
#include "ofxsMacros.h"
#include "ofxsCoords.h"
#include "ofxsMultiThread.h"
// std::mutex rather than the host's: some OFX hosts have no mutexes in the MT-Suite (e.g. Sony Catalyst Edit)
#include <mutex>
#include <cstring>
#include <iostream>
#include <vector>

#if defined(__APPLE__) || defined(__MACOSX)
#include "OpenCL/mac/cl.hpp"
#else
#include <CL/cl.hpp>
#endif

typedef std::mutex OCLMutex;
typedef std::lock_guard<std::mutex> OCLAutoMutex;

#define kParamOCLDevice "device"
#define kParamOCLDeviceLabel "OpenCL Device"
#define kParamOCLDeviceHint "OpenCL Device used for this plugin."

class OCLPluginHelperBase
    : public OFX::ImageEffect
{
public:

    OCLPluginHelperBase(OfxImageEffectHandle handle, const std::string &kernelSource, const std::string &pluginID);
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE;
    static OFX::PageParamDescriptor* describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context);
    static void describeInContextEnd(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context, OFX::PageParamDescriptor* page);
    void setupContext(bool context, std::string kernelString);
    static std::vector<cl::Device> getDevices();
    bool getSelectedDevice(cl::Device* device) const;
    void checkCLError(cl_int err, const char* what);
    template <typename T>
    void setKernelArg(cl::Kernel& kernel, cl_uint index, const T& value)
    {
        checkCLError(kernel.setArg(index, value), "clSetKernelArg");
    }

protected:
    OFX::Clip *_dstClip;
    OFX::Clip *_srcClip;
    OFX::ChoiceParam *_device;
    std::string _source;
    std::string _plugin;
    int _renderscale;

    // Set up by setupContext(), possibly while renders run: only access them
    // with _setupMutex held (render() copies the handles at start).
    OCLMutex _setupMutex;
    cl::Device _clDevice;
    cl::Context _context;
    cl::Program _program;
    std::string _setupError; // why setupContext() failed, empty on success

private:
    std::string buildProgram(bool context, const std::string& kernelString, cl::Device* device, cl::Context* clContext, cl::Program* program);
};

template <int SupportsRenderScale>
class OCLPluginHelper
    : public OCLPluginHelperBase
{
public:

    OCLPluginHelper(OfxImageEffectHandle handle, const std::string &kernelSource, const std::string &pluginID)
        : OCLPluginHelperBase(handle, kernelSource, pluginID)
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
    OFX::auto_ptr<const OFX::Image> srcImg(_srcClip->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        checkBadRenderScaleOrField(srcImg, args);
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
    OFX::auto_ptr<OFX::Image> dstImg(_dstClip->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    checkBadRenderScaleOrField(dstImg, args);

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
    const OfxRectI& renderWindow = args.renderWindow;
    int width = renderWindow.x2 - renderWindow.x1;
    int height = renderWindow.y2 - renderWindow.y1;
    if (width <= 0 || height <= 0) {
        return;
    }

    // copy the handles: changedParam() may call setupContext() concurrently
    cl::Device device;
    cl::Context context;
    cl::Program program;
    std::string setupError;
    {
        OCLAutoMutex lock(_setupMutex);
        device = _clDevice;
        context = _context;
        program = _program;
        setupError = _setupError;
    }

    // the context or program could not be set up (e.g. no device): report why
    // instead of returning an uninitialised image
    if ( !setupError.empty() ) {
        setPersistentMessage(OFX::Message::eMessageError, "", setupError);
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

#ifdef DEBUG
    std::cout << "rendering using OpenCL device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
#endif

    // The kernel works on packed width x height images covering the render
    // window. Images may have padded or negative kOfxImagePropRowBytes, which
    // OpenCL cannot take as a row pitch, and bounds that differ from the render
    // window, so copy the source pixels row by row with getPixelAddress().
    // Source pixels outside the source bounds are transparent black.
    const size_t rowFloats = (size_t)width * 4;
    std::vector<float> inPixels(rowFloats * height, 0.f);
    OfxRectI srcWindow;
    if ( OFX::Coords::rectIntersection(renderWindow, srcBounds, &srcWindow) && (srcWindow.x1 < srcWindow.x2) ) {
        for (int y = srcWindow.y1; y < srcWindow.y2; ++y) {
            std::memcpy(&inPixels[(y - renderWindow.y1) * rowFloats + (srcWindow.x1 - renderWindow.x1) * 4],
                        srcImg->getPixelAddress(srcWindow.x1, y),
                        (srcWindow.x2 - srcWindow.x1) * 4 * sizeof(float));
        }
    }
    std::vector<float> outPixels(rowFloats * height);

    // render
    cl_int err = CL_SUCCESS;
    cl::ImageFormat format(CL_RGBA, CL_FLOAT);
    cl::Image2D in(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, format, width, height, 0, &inPixels[0], &err);
    checkCLError(err, "clCreateImage2D");
    cl::Image2D out(context, CL_MEM_WRITE_ONLY, format, width, height, 0, NULL, &err);
    checkCLError(err, "clCreateImage2D");

    cl::Kernel kernel(program, "filter", &err);
    checkCLError(err, "clCreateKernel");
    setKernelArg(kernel, 0, in);
    setKernelArg(kernel, 1, out);
    render(args, kernel);

    cl::Event timer;
    cl::size_t<3> origin;
    cl::size_t<3> size;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    size[0] = width;
    size[1] = height;
    size[2] = 1;

    cl::CommandQueue queue = cl::CommandQueue(context, device, 0, &err);
    checkCLError(err, "clCreateCommandQueue");
    checkCLError(queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &timer), "clEnqueueNDRangeKernel");
    checkCLError(timer.wait(), "clWaitForEvents");
    checkCLError(queue.enqueueReadImage(out, CL_TRUE, origin, size, 0, 0, &outPixels[0]), "clEnqueueReadImage");
    checkCLError(queue.finish(), "clFinish");

    // the render window is inside the dst bounds (checked above)
    for (int y = renderWindow.y1; y < renderWindow.y2; ++y) {
        std::memcpy(dstImg->getPixelAddress(renderWindow.x1, y),
                    &outPixels[(y - renderWindow.y1) * rowFloats],
                    rowFloats * sizeof(float));
    }
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
