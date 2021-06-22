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

#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include <iostream>
#include <stdint.h>
#include <cmath>
#include "ofxNatron.h"
#include <cstring>

#define kPluginName "PolaroidOFX"
#define kPluginGrouping "Extra/Misc"
#define kPluginIdentifier "net.fxarena.openfx.Polaroid"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 4

#define kParamText "caption"
#define kParamTextLabel "Caption"
#define kParamTextHint "Add caption to polaroid"

#define kParamAngle "angle"
#define kParamAngleLabel "Angle"
#define kParamAngleHint "Adjust polaroid angle"
#define kParamAngleDefault 5

#define kParamFontSize "size"
#define kParamFontSizeLabel "Font size"
#define kParamFontSizeHint "The height of the characters to render in pixels"
#define kParamFontSizeDefault 64

#define kParamFontName "font"
#define kParamFontNameLabel "Font family"
#define kParamFontNameHint "The name of the font to be used"
#define kParamFontNameDefault "Arial"

#if MagickLibVersion >= 0x700
#define kParamFontNameAltDefault "DejaVu-Sans-Book" // failsafe on Linux/BSD
#else
#define kParamFontNameAltDefault "DejaVu-Sans" // failsafe on Linux/BSD
#endif

#define kParamFont "selectedFont"
#define kParamFontLabel "Font"
#define kParamFontHint "Selected font"

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

#define kParamOpenMP "openmp"
#define kParamOpenMPLabel "OpenMP"
#define kParamOpenMPHint "Enable/Disable OpenMP support. This will enable the plugin to use as many threads as allowed by host."
#define kParamOpenMPDefault false

using namespace OFX;
static bool gHostIsNatron = false;
static bool _hasOpenMP = false;

class PolaroidPlugin : public OFX::ImageEffect
{
public:
    PolaroidPlugin(OfxImageEffectHandle handle);
    virtual ~PolaroidPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::StringParam *text_;
    OFX::DoubleParam *angle_;
    OFX::IntParam *fontSize_;
    OFX::ChoiceParam *_fontName;
    OFX::StringParam *_font;
    bool has_freetype;
    OFX::BooleanParam *enableOpenMP_;
};

PolaroidPlugin::PolaroidPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(NULL)
, srcClip_(NULL)
, text_(NULL)
, angle_(NULL)
, fontSize_(NULL)
, _fontName(NULL)
, _font(NULL)
, has_freetype(false)
, enableOpenMP_(NULL)
{
    Magick::InitializeMagick(NULL);

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("freetype") != std::string::npos)
        has_freetype = true;

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    text_ = fetchStringParam(kParamText);
    angle_ = fetchDoubleParam(kParamAngle);
    fontSize_ = fetchIntParam(kParamFontSize);
    _fontName = fetchChoiceParam(kParamFontName);
    _font = fetchStringParam(kParamFont);
    enableOpenMP_ = fetchBooleanParam(kParamOpenMP);

    assert(text_ && angle_ && fontSize_ && _fontName && _font && enableOpenMP_);

    // Setup selected font
    std::string font, fontName;
    _font->getValue(font);
    int fontCount = _fontName->getNOptions();
    if (fontCount > 0) {
        int fontID;
        _fontName->getValue(fontID);
        if (fontID < fontCount) {
            _fontName->getOption(fontID, fontName);
            // cascade menu
            if (fontName.length() > 2 && fontName[1] == '/' && gHostIsNatron) {
                fontName.erase(0,2);
            }
        }
        if (font.empty() && !fontName.empty()) {
            _font->setValue(fontName);
        } else if (fontName != font) { // always prefer font
            for (int x = 0; x < fontCount; ++x) {
                std::string fontFound;
                _fontName->getOption(x, fontFound);
                // cascade menu
                if (fontFound.length() > 2 && fontFound[1] == '/' && gHostIsNatron) {
                    fontFound.erase(0,2);
                }
               if (fontFound == font) {
                    _fontName->setValue(x);
                    break;
                }
            }
        }
    }
}

PolaroidPlugin::~PolaroidPlugin()
{
}

void PolaroidPlugin::render(const OFX::RenderArguments &args)
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

    // font support?
    if (!has_freetype) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Freetype missing");
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
    if (dstComponents != OFX::ePixelComponentRGBA) {
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
    std::string text, font;
    double angle;
    int fontSize;
    bool enableOpenMP = false;
    text_->getValueAtTime(args.time, text);
    angle_->getValueAtTime(args.time, angle);
    fontSize_->getValueAtTime(args.time, fontSize);
    _font->getValueAtTime(args.time, font);
    if ( font.empty() ) { // always prefer font, use fontName if empty
        int fontID;
        _fontName->getValueAtTime(args.time, fontID);
        int fontCount = _fontName->getNOptions();
        if (fontID < fontCount) {
            _fontName->getOption(fontID, font);
            // cascade menu
            if (font.length() > 2 && font[1] == '/' && gHostIsNatron) {
                font.erase(0,2);
            }
        }
    }
    // cascade menu
    if (font.length() > 2 && font[1] == '/' && gHostIsNatron) {
        font.erase(0,2);
    }
    enableOpenMP_->getValueAtTime(args.time, enableOpenMP);

    // setup
    int width = srcRod.x2-srcRod.x1;
    int height = srcRod.y2-srcRod.y1;

    // OpenMP
#ifndef LEGACYIM
    unsigned int threads = 1;
    if (_hasOpenMP && enableOpenMP)
        threads = OFX::MultiThread::getNumCPUs();

    Magick::ResourceLimits::thread(threads);
#endif

    // read image
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,1)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(width,height,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());
    //if (!image.alpha())
        //image.alpha(true);

    // no fonts?
    if ( font.empty() ) {
        font = kParamFontNameDefault;
    }

    // polaroid
    image.flip();
    image.fontPointsize(std::floor(fontSize * args.renderScale.x + 0.5));
    image.borderColor("white"); // TODO param
    image.backgroundColor("black"); // TODO param
    image.font(font);
#if MagickLibVersion >= 0x700
    image.polaroid(text,angle,MagickCore::UndefinedInterpolatePixel);
#else
    image.polaroid(text,angle);
#endif
    image.backgroundColor("none");
    image.flip();
    std::ostringstream scaleW;
    scaleW << width << "x";
    std::ostringstream scaleH;
    scaleH << "x" << height;
    std::size_t columns = width;
    std::size_t rows = height;
    if (image.columns()>columns)
        image.scale(scaleW.str());
    if (image.rows()>rows)
        image.scale(scaleH.str());
    image.extent(Magick::Geometry(width,height),Magick::CenterGravity);

    // return image
    if (dstClip_ && dstClip_->isConnected()) {
        output.composite(image, 0, 0, Magick::OverCompositeOp);
#if MagickLibVersion >= 0x700
        output.composite(image, 0, 0, Magick::CopyAlphaCompositeOp);
#else
        output.composite(image, 0, 0, Magick::CopyOpacityCompositeOp);
#endif
        output.write(0,0,args.renderWindow.x2 - args.renderWindow.x1,args.renderWindow.y2 - args.renderWindow.y1,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
    }
}

void PolaroidPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (paramName == kParamFontName) {
        // set font from fontName
        int fontCount = _fontName->getNOptions();
        if (fontCount > 0) {
            int fontID;
            _fontName->getValueAtTime(args.time, fontID);
            if (fontID < fontCount) {
                std::string fontName;
                _fontName->getOption(fontID, fontName);
                // cascade menu
                if (fontName.length() > 2 && fontName[1] == '/' && gHostIsNatron) {
                    fontName.erase(0,2);
                }
                if ( !fontName.empty() ) {
                    _font->setValue(fontName);
                }
            }
        }
    }

    clearPersistentMessage();
}

bool PolaroidPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(PolaroidPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void PolaroidPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("Polaroid image effect node.");

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
void PolaroidPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    std::string features = MagickCore::GetMagickFeatures();
    if (features.find("OpenMP") != std::string::npos)
        _hasOpenMP = true;

    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

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
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamAngle);
        param->setLabel(kParamAngleLabel);
        param->setHint(kParamAngleHint);
        param->setRange(-360, 360);
        param->setDisplayRange(-60, 60);
        param->setDefault(kParamAngleDefault);
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamText);
        param->setLabel(kParamTextLabel);
        param->setHint(kParamTextHint);
        param->setStringType(eStringTypeMultiLine);
        param->setAnimates(true);
        param->setDefault("Enter text");
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamFontName);
        param->setLabel(kParamFontNameLabel);
        param->setHint(kParamFontNameHint);
        if (gHostIsNatron)
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);

        // Get all fonts
        int defaultFont = 0;
        int altFont = 0;
        char **fonts;
        std::size_t fontList;
        fonts=MagickCore::MagickQueryFonts("*",&fontList);
        for (size_t i=0;i<fontList;i++) {
            std::string fontItem;
            std::string fontName = fonts[i];
            if (gHostIsNatron) {
                fontItem=fontName[0];
                fontItem.append("/"+fontName);
            }
            else
                fontItem=fontName;

            param->appendOption(fontItem);

            std::string fontStr(fonts[i]);
            if (fontStr == kParamFontNameDefault) {
                defaultFont = i;
            } else if (fontStr == kParamFontNameAltDefault) {
                altFont = i;
            }
        }
        for (size_t i = 0; i < fontList; i++) {
            MagickCore::MagickRelinquishMemory(fonts[i]);
            fonts[i] = NULL;
        }
        MagickCore::MagickRelinquishMemory(fonts);

        if (defaultFont > 0) {
            param->setDefault(defaultFont);
        } else if (defaultFont == 0 && altFont > 0) {
            param->setDefault(altFont);
        }

        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamFont);
        param->setLabel(kParamFontLabel);
        param->setHint(kParamFontHint);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);

        #ifdef DEBUG
        param->setIsSecret(false);
        #else
        param->setIsSecret(true);
        #endif

        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamFontSize);
        param->setLabel(kParamFontSizeLabel);
        param->setHint(kParamFontSizeHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamFontSizeDefault);
        param->setAnimates(true);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamOpenMP);
        param->setLabel(kParamOpenMPLabel);
        param->setHint(kParamOpenMPHint);
        param->setDefault(kParamOpenMPDefault);
        param->setAnimates(false);
        if (!_hasOpenMP)
            param->setEnabled(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* PolaroidPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new PolaroidPlugin(handle);
}

static PolaroidPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
