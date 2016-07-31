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

OCLPluginHelperBase::OCLPluginHelperBase(OfxImageEffectHandle handle, const std::string &kernelSource)
    : ImageEffect(handle)
    , _dstClip(0)
    , _srcClip(0)
    , _clType(0)
    , _clVendor(0)
    , _renderscale(0)
{
    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    assert(_dstClip && _dstClip->getPixelComponents() == OFX::ePixelComponentRGBA);
    _srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(_srcClip && _srcClip->getPixelComponents() == OFX::ePixelComponentRGBA);

    _clType = fetchChoiceParam(kParamCLType);
    _clVendor = fetchChoiceParam(kParamCLVendor);

    _source = kernelSource;

    assert(_clType && _clVendor);

    setupContext();
}

void
OCLPluginHelperBase::setupContext()
{
    int type, vendor;
    cl_device_type CLtype;
    cl_vendor CLvendor;

    _clType->getValue(type);
    _clVendor->getValue(vendor);

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

    _context = createCLContext(CLtype, CLvendor);
    if (!_source.empty()) {
        _program = buildProgramFromString(_context, _source);
    }
}

void
OCLPluginHelperBase::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!_renderscale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (paramName == kParamCLType || paramName == kParamCLVendor) {
        setupContext();
    }

    clearPersistentMessage();
}

OFX::PageParamDescriptor*
OCLPluginHelperBase::describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context)
{
    OFX::ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(false);
    srcClip->setIsMask(false);

    OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->setSupportsTiles(false);
}

void
OCLPluginHelperBase::describeInContextEnd(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/, OFX::PageParamDescriptor* page)
{
    {
        OFX::ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamCLType);
        param->setLabel(kParamCLTypeLabel);
        param->setHint(kParamCLTypeHint);
        param->setAnimates(false);
        param->setDefault(kParamCLTypeDefault);
        param->appendOption("Any");
        param->appendOption("GPU");
        param->appendOption("CPU");
        if (page) {
            page->addChild(*param);
        }
    }
    {
        OFX::ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamCLVendor);
        param->setLabel(kParamCLVendorLabel);
        param->setHint(kParamCLVendorHint);
        param->setAnimates(false);
        param->setDefault(kParamCLVendorDefault);
        param->appendOption("Any");
        param->appendOption("NVIDIA");
        param->appendOption("AMD");
        param->appendOption("Intel");
        if (page) {
            page->addChild(*param);
        }
    }
}
