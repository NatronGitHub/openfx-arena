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

#include "MagickPlugin.h"
#include <iostream>

using namespace OFX;

MagickPluginHelperBase::MagickPluginHelperBase(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , _dstClip(NULL)
    , _srcClip(NULL)
    , _enableMP(NULL)
    , _matte(NULL)
    , _vpixel(NULL)
{
    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    assert(_dstClip && _dstClip->getPixelComponents() == OFX::ePixelComponentRGBA);
    _srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(_srcClip && _srcClip->getPixelComponents() == OFX::ePixelComponentRGBA);

    _enableMP = fetchBooleanParam(kParamOpenMP);
    _matte = fetchBooleanParam(kParamMatte);
    _vpixel = fetchChoiceParam(kParamVPixel);
    assert(_enableMP && _matte && _vpixel);
}

void
MagickPluginHelperBase::changedParam(const OFX::InstanceChangedArgs &args, const std::string &/*paramName*/)
{
    if (!_renderscale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    clearPersistentMessage();
}

OFX::PageParamDescriptor*
MagickPluginHelperBase::describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
    OFX::ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(false);
    srcClip->setIsMask(false);

    OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->setSupportsTiles(false);

    std::string features = MagickCore::GetMagickFeatures();
    if (features.find("OpenMP") != std::string::npos) {
        _hasMP = true;
    }

    OFX::PageParamDescriptor *page = desc.definePageParam("Controls");
    {
        OFX::BooleanParamDescriptor *param = desc.defineBooleanParam(kParamOpenMP);
        param->setLabel(kParamOpenMPLabel);
        param->setHint(kParamOpenMPHint);
        param->setDefault(kParamOpenMPDefault);
        param->setAnimates(false);
        if (!_hasMP) {
            param->setEnabled(false);
        }
        if (page) {
            page->addChild(*param);
        }
    }
    {
        OFX::BooleanParamDescriptor *param = desc.defineBooleanParam(kParamMatte);
        param->setLabel(kParamMatteLabel);
        param->setHint(kParamMatteHint);
        param->setDefault(kParamMatteDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        OFX::ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamVPixel);
        param->setLabel(kParamVPixelLabel);
        param->setHint(kParamVPixelHint);
        param->appendOption("Undefined");
        param->appendOption("Background");
        param->appendOption("Black");
        param->appendOption("CheckerTile");
        param->appendOption("Dither");
        param->appendOption("Edge");
        param->appendOption("Gray");
        param->appendOption("HorizontalTile");
        param->appendOption("HorizontalTileEdge");
        param->appendOption("Mirror");
        param->appendOption("Random");
        param->appendOption("Tile");
        param->appendOption("Transparent");
        param->appendOption("VerticalTile");
        param->appendOption("VerticalTileEdge");
        param->appendOption("White");
        param->setDefault(kParamVPixelDefault);
        param->setAnimates(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    return page;
}

void
MagickPluginHelperBase::describeInContextEnd(OFX::ImageEffectDescriptor &/*desc*/, OFX::ContextEnum /*context*/, OFX::PageParamDescriptor* /*page*/)
{
}
