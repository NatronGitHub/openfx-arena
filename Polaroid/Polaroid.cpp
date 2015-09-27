/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "Polaroid.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include <Magick++.h>
#include <iostream>
#include <stdint.h>
#include <cmath>
#include "ofxNatron.h"
#include <cstring>

#define kPluginName "PolaroidOFX"
#define kPluginGrouping "Arena"
#define kPluginIdentifier "net.fxarena.openfx.Polaroid"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 1
#define kPluginMagickVersion 26640

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
#define kParamFontNameAltDefault "DejaVu-Sans" // failsafe on Linux/BSD

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

using namespace OFX;
static bool gHostIsNatron = false;

class PolaroidPlugin : public OFX::ImageEffect
{
public:
    PolaroidPlugin(OfxImageEffectHandle handle);
    virtual ~PolaroidPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::StringParam *text_;
    OFX::DoubleParam *angle_;
    OFX::IntParam *fontSize_;
    OFX::ChoiceParam *fontName_;
    bool has_fontconfig;
    bool has_freetype;
};

PolaroidPlugin::PolaroidPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
,has_fontconfig(false)
,has_freetype(false)
{
    Magick::InitializeMagick(NULL);

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("fontconfig") != std::string::npos)
        has_fontconfig = true;
    if (delegates.find("freetype") != std::string::npos)
        has_freetype = true;

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    text_ = fetchStringParam(kParamText);
    angle_ = fetchDoubleParam(kParamAngle);
    fontSize_ = fetchIntParam(kParamFontSize);
    fontName_ = fetchChoiceParam(kParamFontName);

    assert(text_ && angle_ && fontSize_ && fontName_);
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

    // font support?
    if (!has_fontconfig||!has_freetype) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Fontconfig and/or Freetype missing");
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
    std::string text,fontName;
    double angle;
    int fontSize,fontID;
    text_->getValueAtTime(args.time, text);
    angle_->getValueAtTime(args.time, angle);
    fontSize_->getValueAtTime(args.time, fontSize);
    fontName_->getValueAtTime(args.time, fontID);
    fontName_->getOption(fontID,fontName);

    if (gHostIsNatron)
        fontName.erase(0,2);

    // setup
    int width = srcRod.x2-srcRod.x1;
    int height = srcRod.y2-srcRod.y1;

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    // read image
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(width,height,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());

    #ifdef DEBUG_MAGICK
    image.debug(true);
    #endif

    // no fonts?
    if (fontName.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No fonts found, please check installation");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // polaroid
    image.flip();
    image.fontPointsize(std::floor(fontSize * args.renderScale.x + 0.5));
    image.borderColor("white"); // TODO param
    image.backgroundColor("black"); // TODO param
    // TODO add stuff from Text.ofx
    image.font(fontName);
    image.polaroid(text,angle);
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
    if (dstClip_ && dstClip_->isConnected() && srcClip_ && srcClip_->isConnected())
        image.write(0,0,width,height,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
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
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    if (magickNumber != kPluginMagickVersion)
        magickString.append("\n\nWarning! You are using an unsupported version of ImageMagick.");
    std::string delegates = MagickCore::GetMagickDelegates();
    desc.setPluginDescription("Polaroid image effect node.\n\nPowered by "+magickString+"\n\nFeatures: "+delegates);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void PolaroidPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    gHostIsNatron = (OFX::getImageEffectHostDescription()->hostName == kNatronOfxHostName);

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
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
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

            if (std::strcmp(fonts[i],kParamFontNameDefault)==0)
                defaultFont=i;
            if (std::strcmp(fonts[i],kParamFontNameAltDefault)==0)
                altFont=i;
        }
        for (size_t i = 0; i < fontList; i++)
            free(fonts[i]);

        if (defaultFont>0)
            param->setDefault(defaultFont);
        else if (defaultFont==0&&altFont>0)
            param->setDefault(altFont);

        param->setAnimates(true);
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
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* PolaroidPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new PolaroidPlugin(handle);
}

void getPolaroidPluginID(OFX::PluginFactoryArray &ids)
{
    static PolaroidPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
