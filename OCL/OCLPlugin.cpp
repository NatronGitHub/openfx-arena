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

#include <fstream>
#include <iostream>
#include <sstream>

// Bundle path, cached at describe time: per ofxCore.h, kOfxPluginPropFilePath
// is an effect descriptor property ("Property Set - effect descriptor (read only)"),
// so it is not guaranteed to exist on instances.
static std::string gBundlePath;

OCLPluginHelperBase::OCLPluginHelperBase(OfxImageEffectHandle handle, const std::string &kernelSource, const std::string &pluginID)
    : ImageEffect(handle)
    , _dstClip(NULL)
    , _srcClip(NULL)
    , _device(NULL)
    , _renderscale(0)
{
    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    assert(_dstClip && _dstClip->getPixelComponents() == OFX::ePixelComponentRGBA);
    _srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(_srcClip && _srcClip->getPixelComponents() == OFX::ePixelComponentRGBA);

    _device = fetchChoiceParam(kParamOCLDevice);
    _source = kernelSource;
    _plugin = pluginID;

    assert(_device);

    setupContext(true, "");
}

// Create the context (if requested) and build the program into local handles,
// then publish them under _setupMutex: this runs from the constructor and from
// changedParam() while renders (eRenderFullySafe) may be reading them.
void
OCLPluginHelperBase::setupContext(bool context, std::string kernelString)
{
    cl::Device device;
    cl::Context clContext;
    cl::Program program;
    if (!context) {
        OCLAutoMutex lock(_setupMutex);
        clContext = _context;
    }
    std::string error = buildProgram(context, kernelString, &device, &clContext, &program);
    {
        OCLAutoMutex lock(_setupMutex);
        _clDevice = device;
        _context = clContext;
        _program = program;
        _setupError = error;
    }
    if ( !error.empty() ) {
        // render() fails with this message instead of running an invalid program
        std::cerr << _plugin << ": " << error << std::endl;
        setPersistentMessage(OFX::Message::eMessageError, "", error);
    }
}

// Returns an error message, or an empty string on success.
std::string
OCLPluginHelperBase::buildProgram(bool context, const std::string& kernelString, cl::Device* device, cl::Context* clContext, cl::Program* program)
{
    if ( !getSelectedDevice(device) ) {
        return "No OpenCL device found";
    }
    cl_int err = CL_SUCCESS;
    if (context) {
        cl::Platform platform(device->getInfo<CL_DEVICE_PLATFORM>());
        cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(), 0};
        *clContext = cl::Context(CL_DEVICE_TYPE_ALL, properties, NULL, NULL, &err);
        if (err != CL_SUCCESS) {
            std::ostringstream msg;
            msg << "Failed to create OpenCL context (error " << err << ")";
            return msg.str();
        }
#ifdef DEBUG
        std::cout << "setup OpenCL context for: " << platform.getInfo<CL_PLATFORM_VENDOR>() << std::endl;
#endif
    }

    std::string kSource;
    if (!kernelString.empty()) {
        kSource = kernelString;
    } else if (!_source.empty()) {
       kSource = _source;
    } else if (!_plugin.empty()) {
        std::string kSourcePath = gBundlePath;
        if ( kSourcePath.empty() ) {
            // fallback for hosts that also set it on the instance
            kSourcePath = getPropertySet().propGetString(kOfxPluginPropFilePath, false);
        }
        kSourcePath.append("/Contents/Resources/");
        kSourcePath.append(_plugin);
        kSourcePath.append(".cl");
        std::ifstream f;
        f.open(kSourcePath.c_str());
        std::ostringstream s;
        s << f.rdbuf();
        f.close();
        if (!s.str().empty()) {
            kSource = s.str();
        } else {
            return "Failed to read OpenCL kernel " + kSourcePath;
        }
    }
    if (!kSource.empty()) {
        cl::Program::Sources sources(1, std::make_pair(kSource.c_str(), kSource.length()+1));
        *program = cl::Program(*clContext, sources, &err);
        if (err != CL_SUCCESS) {
            std::ostringstream msg;
            msg << "Failed to create OpenCL program (error " << err << ")";
            return msg.str();
        }
        std::string buildOptions;

        std::vector<cl::Device> platformDevices = clContext->getInfo<CL_CONTEXT_DEVICES>();
        err = program->build(platformDevices, buildOptions.c_str());
        if (err != CL_SUCCESS) {
            std::ostringstream msg;
            msg << "Failed to build OpenCL kernel (error " << err << "):\n"
                << program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device);
            return msg.str();
        }
    }

    return std::string();
}

// Set a persistent error message and throw kOfxStatFailed if an OpenCL call failed.
void
OCLPluginHelperBase::checkCLError(cl_int err, const char* what)
{
    if (err != CL_SUCCESS) {
        std::ostringstream msg;
        msg << what << " failed (OpenCL error " << err << ")";
        setPersistentMessage( OFX::Message::eMessageError, "", msg.str() );
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }
}

std::vector<cl::Device>
OCLPluginHelperBase::getDevices()
{
    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> devices;
    cl::Platform::get(&platforms);
    for (size_t i=0; i < platforms.size(); i++) {
        std::vector<cl::Device> platformDevices;
        platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &platformDevices);
        for (size_t i=0; i < platformDevices.size(); i++) {
            devices.push_back(platformDevices[i]);
        }
    }
    return devices;
}

// Get the device selected by the device choice param. The index is clamped to the
// devices present on this machine, since a project saved on a machine with more
// OpenCL devices may store an index that is out of range here.
// Returns false if there is no OpenCL device at all.
bool
OCLPluginHelperBase::getSelectedDevice(cl::Device* device) const
{
    std::vector<cl::Device> devices = getDevices();
    if ( devices.empty() ) {
        return false;
    }
    int index = 0;
    _device->getValue(index);
    if (index < 0) {
        index = 0;
    } else if ( index >= (int)devices.size() ) {
        index = (int)devices.size() - 1;
    }
    *device = devices[index];

    return true;
}

void
OCLPluginHelperBase::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!_renderscale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    // clear first: setupContext() may set a new error message
    clearPersistentMessage();
    if (paramName == kParamOCLDevice) {
        setupContext(true, "");
    }
}

OFX::PageParamDescriptor*
OCLPluginHelperBase::describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
    gBundlePath = desc.getPropertySet().propGetString(kOfxPluginPropFilePath, false);

    OFX::ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(false);
    srcClip->setIsMask(false);

    OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->setSupportsTiles(false);

    OFX::PageParamDescriptor *page = desc.definePageParam("Controls");
    {
        OFX::ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamOCLDevice);
        param->setLabel(kParamOCLDeviceLabel);
        param->setHint(kParamOCLDeviceHint);
        param->setAnimates(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);

        std::vector<cl::Device> devices = getDevices();
        for (size_t i=0; i < devices.size(); i++) {
            std::string device = devices[i].getInfo<CL_DEVICE_NAME>();
            cl::Platform platform(devices[i].getInfo<CL_DEVICE_PLATFORM>());
            device.append(" [");
            device.append(platform.getInfo<CL_PLATFORM_VENDOR>());
            device.append("]");
            param->appendOption(device);
        }

        if (page) {
            page->addChild(*param);
        }
    }
    return page;
}

void
OCLPluginHelperBase::describeInContextEnd(OFX::ImageEffectDescriptor &/*desc*/, OFX::ContextEnum /*context*/, OFX::PageParamDescriptor* /*page*/)
{
}
