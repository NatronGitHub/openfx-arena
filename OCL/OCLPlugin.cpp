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

OCLPluginHelperBase::OCLPluginHelperBase(OfxImageEffectHandle handle, const std::string &kernelSource, const std::string &pluginID)
    : ImageEffect(handle)
    , _dstClip(NULL)
    , _srcClip(NULL)
    , _device(NULL)
    , _renderscale(0)
{
    const ImageEffectHostDescription &hostDescription = *getImageEffectHostDescription();
    _hostIsResolve = (hostDescription.hostName.substr(0, 14) == "DaVinciResolve");  // Resolve gives bad image properties

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

void
OCLPluginHelperBase::setupContext(bool context, std::string kernelString)
{
    int device;
    _device->getValue(device);
    std::vector<cl::Device> devices = getDevices();
    if (devices.size()!=0) {
        if (context) {
            cl::Platform platform(devices[device].getInfo<CL_DEVICE_PLATFORM>());
            cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(), 0};
            _context = cl::Context(CL_DEVICE_TYPE_ALL, properties);
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
            std::string kSourcePath;
            kSourcePath.append(getPropertySet().propGetString(kOfxPluginPropFilePath, false));
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
                std::cout << "Failed to read OpenCL kernel!" << std::endl;
            }
        }
        if (!kSource.empty()) {
            cl::Program::Sources sources(1, std::make_pair(kSource.c_str(), kSource.length()+1));
            _program = cl::Program(_context,sources);
            std::string buildOptions;

            std::vector<cl::Device> platformDevices = _context.getInfo<CL_CONTEXT_DEVICES>();
            if(_program.build(platformDevices, buildOptions.c_str()) == CL_BUILD_PROGRAM_FAILURE){
                setPersistentMessage(OFX::Message::eMessageError, "", "Failed to build kernel, see terminal for log");
                std::cout << "Failed to build OpenCL kernel: "<< _program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[device]) << std::endl;
            }
        }
    } else {
        std::cout << "No OpenCL devices found!" << std::endl;
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

void
OCLPluginHelperBase::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!_renderscale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    if (paramName == kParamOCLDevice) {
        setupContext(true, "");
    }
    clearPersistentMessage();
}

OFX::PageParamDescriptor*
OCLPluginHelperBase::describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
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
