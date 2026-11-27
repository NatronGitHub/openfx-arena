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

#include "MagickCommon.h"
#include "PackedImage.h"

#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include <iostream>

#define kPluginName "ModulateOFX"
#define kPluginGrouping "Extra/Color"
#define kPluginIdentifier "net.fxarena.openfx.Modulate"
#define kPluginVersionMajor 2 // 2: unpremultiply the source before ImageMagick (see PackedImage::PackedSrc)
#define kPluginVersionMinor 0

#define kParamSaturation "saturation"
#define kParamSaturationLabel "Saturation"
#define kParamSaturationHint "Adjust saturation (%)"
#define kParamSaturationDefault 100

#define kParamHue "hue"
#define kParamHueLabel "Hue"
#define kParamHueHint "Adjust hue (%)"
#define kParamHueDefault 100

#define kParamBrightness "brightness"
#define kParamBrightnessLabel "Brightness"
#define kParamBrightnessHint "Adjust brightness (%)"
#define kParamBrightnessDefault 100

#define kParamOpenMP "openmp"
#define kParamOpenMPLabel "OpenMP"
#define kParamOpenMPHint "Kept for old projects; ImageMagick runs on one thread unless the MAGICK_THREAD_LIMIT environment variable is set."
#define kParamOpenMPDefault true

#define kParamOpenCL "opencl"
#define kParamOpenCLLabel "OpenCL"
#define kParamOpenCLHint "Kept for old projects; ImageMagick's OpenCL is off unless the MAGICK_OCL_DEVICE environment variable is set."
#define kParamOpenCLDefault false

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

using namespace OFX;

static bool _hasOpenMP = false;
static bool _hasOpenCL = false;

class ModulatePlugin : public OFX::ImageEffect
{
public:
    ModulatePlugin(OfxImageEffectHandle handle, bool unpremultSrc);
    virtual ~ModulatePlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    bool unpremultSrc_; // false in versions released before the premultiplication fix
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::DoubleParam *brightness_;
    OFX::DoubleParam *hue_;
    OFX::DoubleParam *saturation_;
    OFX::BooleanParam *enableOpenMP_;
    OFX::BooleanParam *enableOpenCL_;
};

ModulatePlugin::ModulatePlugin(OfxImageEffectHandle handle, bool unpremultSrc)
: OFX::ImageEffect(handle)
, unpremultSrc_(unpremultSrc)
, dstClip_(NULL)
, srcClip_(NULL)
{
    magickInitialize();
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    brightness_ = fetchDoubleParam(kParamBrightness);
    hue_ = fetchDoubleParam(kParamHue);
    saturation_ = fetchDoubleParam(kParamSaturation);
    enableOpenMP_ = fetchBooleanParam(kParamOpenMP);
    enableOpenCL_ = fetchBooleanParam(kParamOpenCL);

    assert(brightness_ && hue_ && saturation_ && enableOpenMP_ && enableOpenCL_);
}

ModulatePlugin::~ModulatePlugin()
{
}

void ModulatePlugin::render(const OFX::RenderArguments &args)
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
    OFX::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        checkBadRenderScaleOrField(srcImg, args);
    } else {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // get dest clip
    if (!dstClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);
    OFX::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
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
    if (dstComponents != OFX::ePixelComponentRGBA || (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
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

    // get params
    double brightness, hue, saturation;
    brightness_->getValueAtTime(args.time, brightness);
    hue_->getValueAtTime(args.time, hue);
    saturation_->getValueAtTime(args.time, saturation);

    // setup
    int width = srcRod.x2-srcRod.x1;
    int height = srcRod.y2-srcRod.y1;

    // read image
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,1)"));
    if (srcClip_ && srcClip_->isConnected()) {
        // ImageMagick expects straight colour
        PackedImage::PackedSrc srcPixels(srcImg.get(), srcRod,
                                         unpremultSrc_ && srcImg->getPreMultiplication() == OFX::eImagePreMultiplied);
        image.read(width,height,"RGBA",Magick::FloatPixel,srcPixels.data());
    }

    // modulate
    image.modulate(brightness,saturation,hue);

    // return image
    if (dstClip_ && dstClip_->isConnected()) {
        output.composite(image, 0, 0, Magick::OverCompositeOp);
#if MagickLibVersion >= 0x700
        output.composite(image, 0, 0, Magick::CopyAlphaCompositeOp);
#else
        output.composite(image, 0, 0, Magick::CopyOpacityCompositeOp);
#endif
        PackedImage::PackedDst dstPixels(dstImg.get(), args.renderWindow);
        output.write(0,0,args.renderWindow.x2 - args.renderWindow.x1,args.renderWindow.y2 - args.renderWindow.y1,"RGBA",Magick::FloatPixel,dstPixels.data());
        dstPixels.commit();
    }
}

bool ModulatePlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactoryVersioned(ModulatePluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
template<unsigned int majorVersion>
void ModulatePluginFactory<majorVersion>::describe(OFX::ImageEffectDescriptor &desc)
{
    if (majorVersion < kPluginVersionMajor) {
        desc.setIsObsolete(true);
    }
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("Modulate color node.");

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
template<unsigned int majorVersion>
void ModulatePluginFactory<majorVersion>::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    std::string features = MagickCore::GetMagickFeatures();
    if (features.find("OpenMP") != std::string::npos)
        _hasOpenMP = true;
    if (features.find("OpenCL") != std::string::npos)
        _hasOpenCL = true;

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

    // make some pages
    PageParamDescriptor *page = desc.definePageParam("Controls");
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamBrightness);
        param->setLabel(kParamBrightnessLabel);
        param->setHint(kParamBrightnessHint);
        param->setRange(0, 200);
        param->setDisplayRange(0, 200);
        param->setDefault(kParamBrightnessDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSaturation);
        param->setLabel(kParamSaturationLabel);
        param->setHint(kParamSaturationHint);
        param->setRange(0, 200);
        param->setDisplayRange(0, 200);
        param->setDefault(kParamSaturationDefault);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamHue);
        param->setLabel(kParamHueLabel);
        param->setHint(kParamHueHint);
        param->setRange(0, 200);
        param->setDisplayRange(0, 200);
        param->setDefault(kParamHueDefault);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamOpenMP);
        param->setLabel(kParamOpenMPLabel);
        param->setHint(kParamOpenMPHint);
        param->setDefault(kParamOpenMPDefault);
        param->setIsSecret(true);
        param->setAnimates(false);
        if (!_hasOpenMP)
            param->setEnabled(false);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamOpenCL);
        param->setLabel(kParamOpenCLLabel);
        param->setHint(kParamOpenCLHint);
        param->setDefault(kParamOpenCLDefault);
        param->setIsSecret(true);
        param->setAnimates(false);
        if (!_hasOpenCL)
            param->setEnabled(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
template<unsigned int majorVersion>
ImageEffect* ModulatePluginFactory<majorVersion>::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    // versions before 2 fed premultiplied pixels to ImageMagick: keep their output
    return new ModulatePlugin(handle, majorVersion >= 2);
}

static ModulatePluginFactory<kPluginVersionMajor> p(kPluginIdentifier, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
// version 1, kept so that existing projects render as before
static ModulatePluginFactory<1> p1(kPluginIdentifier, 2);
mRegisterPluginFactoryInstance(p1)
