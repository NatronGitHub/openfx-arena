/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "TextPango.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include <Magick++.h>
#include <sstream>
#include <iostream>
#include <stdint.h>

#define kPluginName "TextPangoOFX"
#define kPluginGrouping "Draw"
#define kPluginIdentifier "net.fxarena.openfx.TextPango"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 3
#define kPluginMagickVersion 26640

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 0
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

#define kParamText "markup"
#define kParamTextLabel "Markup"
#define kParamTextHint "Pango Markup Language.\n\n Example:\n<span color=\"white\" size=\"x-large\">Text</span>\n<span font=\"Sans Italic 12\" color=\"white\">Text</span>\n\nOr use a text file with markup:\n@/path/to/text_file.txt"

#define kParamGravity "gravity"
#define kParamGravityLabel "Gravity hint"
#define kParamGravityHint "Gravity hint"
#define kParamGravityDefault 0

#define kParamHinting "hinting"
#define kParamHintingLabel "Hinting"
#define kParamHintingHint "Hinting"
#define kParamHintingDefault 0

#define kParamIndent "indent"
#define kParamIndentLabel "Indent"
#define kParamIndentHint "Indent"
#define kParamIndentDefault 0

#define kParamJustify "justify"
#define kParamJustifyLabel "Justify"
#define kParamJustifyHint "Justify"
#define kParamJustifyDefault false

#define kParamWrap "wrap"
#define kParamWrapLabel "Wrap"
#define kParamWrapHint "Wrap"
#define kParamWrapDefault 0

#define kParamEllipsize "ellipsize"
#define kParamEllipsizeLabel "Ellipsize"
#define kParamEllipsizeHint "Ellipsize"
#define kParamEllipsizeDefault 0

#define kParamSingle "single"
#define kParamSingleLabel "Single-paragraph"
#define kParamSingleHint "Single-paragraph"
#define kParamSingleDefault false

#define kParamWidth "width"
#define kParamWidthLabel "Width"
#define kParamWidthHint "Set canvas width, default (0) is project format"
#define kParamWidthDefault 0

#define kParamHeight "height"
#define kParamHeightLabel "Height"
#define kParamHeightHint "Set canvas height, default (0) is project format"
#define kParamHeightDefault 0

using namespace OFX;

class TextPangoPlugin : public OFX::ImageEffect
{
public:
    TextPangoPlugin(OfxImageEffectHandle handle);
    virtual ~TextPangoPlugin();

    /* Override the render */
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;

    /* override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;

    // override the rod call
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;

private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *dstClip_;
    OFX::StringParam *text_;
    OFX::IntParam *width_;
    OFX::IntParam *height_;
    //OFX::ChoiceParam *gravity_;
    OFX::ChoiceParam *hinting_;
    OFX::IntParam *indent_;
    OFX::BooleanParam *justify_;
    OFX::ChoiceParam *wrap_;
    //OFX::ChoiceParam *ellipsize_;
    OFX::BooleanParam *single_;
    bool has_pango;
    bool has_fontconfig;
    bool has_freetype;
};

TextPangoPlugin::TextPangoPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, width_(0)
, height_(0)
{
    Magick::InitializeMagick(NULL);

    has_pango = false;
    has_fontconfig = false;
    has_freetype = false;

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("fontconfig") != std::string::npos)
        has_fontconfig = true;
    if (delegates.find("freetype") != std::string::npos)
        has_freetype = true;
    if (delegates.find("pango") != std::string::npos)
        has_pango = true;

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    text_ = fetchStringParam(kParamText);
    width_ = fetchIntParam(kParamWidth);
    height_ = fetchIntParam(kParamHeight);
    //gravity_ = fetchChoiceParam(kParamGravity);
    hinting_ = fetchChoiceParam(kParamHinting);
    indent_ = fetchIntParam(kParamIndent);
    justify_ = fetchBooleanParam(kParamJustify);
    wrap_ = fetchChoiceParam(kParamWrap);
    //ellipsize_ = fetchChoiceParam(kParamEllipsize);
    single_ = fetchBooleanParam(kParamSingle);

    assert(text_ && width_ && height_ /*&& gravity_*/ && hinting_ && indent_ && justify_ && wrap_ /*&& ellipsize_*/ && single_);
}

TextPangoPlugin::~TextPangoPlugin()
{
}

/* Override the render */
void TextPangoPlugin::render(const OFX::RenderArguments &args)
{
    // renderscale
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // dstclip
    if (!dstClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);

    // get dstclip
    std::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // font support?
    if (!has_fontconfig||!has_freetype||!has_pango) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Fontconfig/Freetype/Pango is missing");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // renderscale
    if (dstImg->getRenderScale().x != args.renderScale.x ||
        dstImg->getRenderScale().y != args.renderScale.y ||
        dstImg->getField() != args.fieldToRender) {
        setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get bitdepth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get channels
    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != OFX::ePixelComponentRGBA) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    OfxRectI dstRod = dstImg->getRegionOfDefinition();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Get params
    std::string text;
    int /*gravity,*/ hinting, indent, wrap/*, ellipsize*/;
    bool justify = false;
    bool single = false;
    text_->getValueAtTime(args.time, text);
    //gravity_->getValueAtTime(args.time, gravity);
    hinting_->getValueAtTime(args.time, hinting);
    indent_->getValueAtTime(args.time, indent);
    justify_->getValueAtTime(args.time, justify);
    wrap_->getValueAtTime(args.time, wrap);
    //ellipsize_->getValueAtTime(args.time, ellipsize);

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    // Generate empty image
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    #ifdef DEBUG
    image.debug(true);
    #endif

    // check for fonts
    std::string fontFile;
    char **fonts;
    std::size_t fontList;
    fonts=MagickCore::MagickQueryFonts("*",&fontList);
    fontFile = fonts[0];
    for (size_t i = 0; i < fontList; i++)
        free(fonts[i]);
    if (fontFile.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No fonts found, please check installation");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    try {
        image.backgroundColor("none"); // must be set to avoid bg
    }
    catch(Magick::Warning &warning) { // ignore since warns interupt render
        #ifdef DEBUG
        std::cout << warning.what() << std::endl;
        #endif
    }

    image.backgroundColor("none"); // must be set to avoid bg

    /*switch(gravity) {
    case 1: // natural
        image.defineValue("pango","gravity-hint","natural");
        break;
    case 2: // strong
        image.defineValue("pango","gravity-hint","strong");
        break;
    case 3: // line
        image.defineValue("pango","gravity-hint","line");
        break;
    }*/
    switch(hinting) {
    case 0:
        image.defineValue("pango","hinting","none");
        break;
    case 1:
        image.defineValue("pango","hinting","auto");
        break;
    case 2:
        image.defineValue("pango","hinting","full");
        break;
    }
    switch(wrap) {
    case 0:
        image.defineValue("pango","wrap","word");
        break;
    case 1:
        image.defineValue("pango","wrap","char");
        break;
    case 2:
        image.defineValue("pango","wrap","word-char");
        break;
    }
    /*switch(ellipsize) {
    case 1:
        image.defineValue("pango","ellipsize","start");
        break;
    case 2:
        image.defineValue("pango","ellipsize","middle");
        break;
    case 3:
        image.defineValue("pango","ellipsize","end");
        break;
    }*/
    if (indent!=0) {
        std::ostringstream indentSize;
        indentSize << indent;
        image.defineValue("pango","indent",indentSize.str());
    }
    if (justify)
        image.defineValue("pango","justify","true");
    if (single)
        image.defineValue("pango","single-paragraph","true");
    try {
        image.read("pango:"+text);
    }
    catch (Magick::Exception &error_) {
        setPersistentMessage(OFX::Message::eMessageError, "", error_.what());
    }

    // Flip image
    image.flip();

    // return image
    if (dstClip_ && dstClip_->isConnected())
        image.write(0,0,width,height,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
}

void TextPangoPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &/*paramName*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    clearPersistentMessage();
}

bool TextPangoPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }

    int width,height;
    width_->getValue(width);
    height_->getValue(height);

    if (width>0 && height>0) {
        rod.x1 = rod.y1 = 0;
        rod.x2 = width;
        rod.y2 = height;
    }
    else {
        rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
    }

    return true;
}

mDeclarePluginFactory(TextPangoPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void TextPangoPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    if (magickNumber != kPluginMagickVersion)
        magickString.append("\n\nWarning! You are using an unsupported version of ImageMagick.");
    std::string delegates = MagickCore::GetMagickDelegates();
    desc.setPluginDescription("Pango text generator node.\n\nPowered by "+magickString+"\n\nFeatures: "+delegates+"\n\nVisit https://github.com/olear/openfx-arena/wiki/Pango for more info regarding usage of the TextPango node.");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // add other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TextPangoPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{   
    // there has to be an input clip, even for generators
    ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setOptional(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamText);
        param->setLabel(kParamTextLabel);
        param->setHint(kParamTextHint);
        param->setStringType(eStringTypeMultiLine);
        param->setAnimates(true);
        param->setDefault("<span color=\"white\" font=\"Sans 100\">Enter <span color=\"red\">text</span></span>");
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamWidth);
        param->setLabel(kParamWidthLabel);
        param->setHint(kParamWidthHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamWidthDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamHeight);
        param->setLabel(kParamHeightLabel);
        param->setHint(kParamHeightHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamHeightDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamIndent);
        param->setLabel(kParamIndentLabel);
        param->setHint(kParamIndentHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamIndentDefault);
        page->addChild(*param);
    }
    /*{
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamGravity);
        param->setLabel(kParamGravityLabel);
        param->setHint(kParamGravityHint);
        param->appendOption("Default");
        param->appendOption("Natural");
        param->appendOption("Strong");
        param->appendOption("Line");
        param->setAnimates(true);
        page->addChild(*param);
    }*/
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamHinting);
        param->setLabel(kParamHintingLabel);
        param->setHint(kParamHintingHint);
        param->appendOption("None");
        param->appendOption("Auto");
        param->appendOption("Full");
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamWrap);
        param->setLabel(kParamWrapLabel);
        param->setHint(kParamWrapHint);
        param->appendOption("Word");
        param->appendOption("Char");
        param->appendOption("Word-Char");
        param->setAnimates(true);
        page->addChild(*param);
    }
    /*{
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamEllipsize);
        param->setLabel(kParamEllipsizeLabel);
        param->setHint(kParamEllipsizeHint);
        param->appendOption("Default");
        param->appendOption("Start");
        param->appendOption("Middle");
        param->appendOption("End");
        param->setAnimates(true);
        page->addChild(*param);
    }*/
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamJustify);
        param->setLabel(kParamJustifyLabel);
        param->setHint(kParamJustifyHint);
        param->setDefault(kParamJustifyDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamSingle);
        param->setLabel(kParamSingleLabel);
        param->setHint(kParamSingleHint);
        param->setDefault(kParamSingleDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TextPangoPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TextPangoPlugin(handle);
}

void getTextPangoPluginID(OFX::PluginFactoryArray &ids)
{
    static TextPangoPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
