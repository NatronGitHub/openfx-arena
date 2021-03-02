/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
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

#ifndef MagickPlugin_h
#define MagickPlugin_h

#include "ofxsImageEffect.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include <Magick++.h>

#define kParamOpenMP "openmp"
#define kParamOpenMPLabel "OpenMP"
#define kParamOpenMPHint "Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host."
#define kParamOpenMPDefault false

#define kParamMatte "matte"
#define kParamMatteLabel "Matte"
#define kParamMatteHint "Merge Alpha before applying effect."
#define kParamMatteDefault false

#define kParamVPixel "vpixel"
#define kParamVPixelLabel "Virtual Pixel"
#define kParamVPixelHint "Virtual Pixel Method."
#define kParamVPixelDefault 12

#define MagickMemoryLimit 4200000000

static bool _hasMP = false;

class MagickPluginHelperBase
    : public OFX::ImageEffect
{
public:

    MagickPluginHelperBase(OfxImageEffectHandle handle);
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE;
    static OFX::PageParamDescriptor* describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context);
    static void describeInContextEnd(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context, OFX::PageParamDescriptor* page);

protected:
    OFX::Clip *_dstClip;
    OFX::Clip *_srcClip;
    OFX::BooleanParam *_enableMP;
    OFX::BooleanParam *_matte;
    OFX::ChoiceParam *_vpixel;
    int _renderscale;
};

template <int SupportsRenderScale>
class MagickPluginHelper
    : public MagickPluginHelperBase
{
public:

    MagickPluginHelper(OfxImageEffectHandle handle)
        : MagickPluginHelperBase(handle)
    {
        _renderscale = SupportsRenderScale;

        // set ImageMagick resources
        Magick::InitializeMagick(NULL);
        Magick::ResourceLimits::disk(0);
        //Magick::ResourceLimits::memory(MagickMemoryLimit);
        //Magick::ResourceLimits::map(MagickMemoryLimit);
    }

    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
    virtual void render(const OFX::RenderArguments &args, Magick::Image &image) = 0;
    static OFX::PageParamDescriptor* describeInContextBegin(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum context)
    {
        return MagickPluginHelperBase::describeInContextBegin(desc, context);
    }
};

template <int SupportsRenderScale>
void MagickPluginHelper<SupportsRenderScale>::render(const OFX::RenderArguments &args)
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
    int width = args.renderWindow.x2 - args.renderWindow.x1;
    int height = args.renderWindow.y2 - args.renderWindow.y1;

    // params
    bool enableMP, matte;
    int vpixel;
    _enableMP->getValueAtTime(args.time, enableMP);
    _matte->getValueAtTime(args.time, matte);
    _vpixel->getValueAtTime(args.time, vpixel);

    // OpenMP
#ifndef DISABLE_OPENMP
    unsigned int threads = 1;
    if (_hasMP && enableMP) {
        threads = OFX::MultiThread::getNumCPUs();
    }
    Magick::ResourceLimits::thread(threads);
#endif

    // render
    Magick::Image image(Magick::Geometry(width, height), Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(width, height), Magick::Color("rgba(0,0,0,1)"));
    if (_srcClip && _srcClip->isConnected()) {
        image.read(width, height, "RGBA", Magick::FloatPixel, (float*)srcImg->getPixelData());
        image.flip();
        switch (vpixel) {
        case 0:
            image.virtualPixelMethod(Magick::UndefinedVirtualPixelMethod);
            break;
        case 1:
            image.virtualPixelMethod(Magick::BackgroundVirtualPixelMethod);
            break;
        case 2:
            image.virtualPixelMethod(Magick::BlackVirtualPixelMethod);
            break;
        case 3:
            image.virtualPixelMethod(Magick::CheckerTileVirtualPixelMethod);
            break;
        case 4:
            image.virtualPixelMethod(Magick::DitherVirtualPixelMethod);
            break;
        case 5:
            image.virtualPixelMethod(Magick::EdgeVirtualPixelMethod);
            break;
        case 6:
            image.virtualPixelMethod(Magick::GrayVirtualPixelMethod);
            break;
        case 7:
            image.virtualPixelMethod(Magick::HorizontalTileVirtualPixelMethod);
            break;
        case 8:
            image.virtualPixelMethod(Magick::HorizontalTileEdgeVirtualPixelMethod);
            break;
        case 9:
            image.virtualPixelMethod(Magick::MirrorVirtualPixelMethod);
            break;
        case 10:
            image.virtualPixelMethod(Magick::RandomVirtualPixelMethod);
            break;
        case 11:
            image.virtualPixelMethod(Magick::TileVirtualPixelMethod);
            break;
        case 12:
            image.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
            break;
        case 13:
            image.virtualPixelMethod(Magick::VerticalTileVirtualPixelMethod);
            break;
        case 14:
            image.virtualPixelMethod(Magick::VerticalTileEdgeVirtualPixelMethod);
            break;
        case 15:
            image.virtualPixelMethod(Magick::WhiteVirtualPixelMethod);
            break;
        }
        if (matte) {
#if MagickLibVersion >= 0x700
            image.alpha(false);
            image.alpha(true);
#else
            image.matte(false);
            image.matte(true);
#endif
        }
        render(args, image);
        image.flip();
    }
    if (_dstClip && _dstClip->isConnected()) {
        output.composite(image, 0, 0, Magick::OverCompositeOp);
#if MagickLibVersion >= 0x700
        output.composite(image, 0, 0, Magick::CopyAlphaCompositeOp);
#else
        output.composite(image, 0, 0, Magick::CopyOpacityCompositeOp);
#endif
        output.write(0, 0, args.renderWindow.x2 - args.renderWindow.x1,args.renderWindow.y2 - args.renderWindow.y1, "RGBA", Magick::FloatPixel, (float*)dstImg->getPixelData());
    }

}

template <int SupportsRenderScale>
bool MagickPluginHelper<SupportsRenderScale>::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

#endif // MagickPlugin_h
