/*
# Copyright (c) 2015, Ole-André Rodlie <olear@dracolinux.org>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
*/

#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include <Magick++.h>
#include <sstream>
#include <iostream>
#include <stdint.h>
#include <fontconfig/fontconfig.h>
#include "ofxNatron.h"
#include <cmath>
#include <cstring>

#define kPluginName "TextPangoOFX"
#define kPluginGrouping "Draw"
#define kPluginIdentifier "net.fxarena.openfx.TextPango"
#define kPluginVersionMajor 2
#define kPluginVersionMinor 3

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

#define kParamAlign "align"
#define kParamAlignLabel "Align"
#define kParamAlignHint "Text align"

#define kParamFontSize "size"
#define kParamFontSizeLabel "Font size"
#define kParamFontSizeHint "The height of the characters to render in pixels"
#define kParamFontSizeDefault 64

#define kParamFontName "font"
#define kParamFontNameLabel "Font family"
#define kParamFontNameHint "The name of the font to be used"
#define kParamFontNameDefault "Arial"
#define kParamFontNameAltDefault "DejaVu Sans" // failsafe on Linux/BSD

#define kParamFont "selectedFont"
#define kParamFontLabel "Font"
#define kParamFontHint "Selected font"

#define kParamShadowOpacity "shadowOpacity"
#define kParamShadowOpacityLabel "Opacity"
#define kParamShadowOpacityHint "Adjust shadow opacity"
#define kParamShadowOpacityDefault 0

#define kParamShadowSigma "shadowSigma"
#define kParamShadowSigmaLabel "Sigma"
#define kParamShadowSigmaHint "Adjust shadow sigma"
#define kParamShadowSigmaDefault 0.5

#define kParamShadowColor "shadowColor"
#define kParamShadowColorLabel "Color"
#define kParamShadowColorHint "The shadow color to render"

#define kParamShadowX "shadowX"
#define kParamShadowXLabel "Offset X"
#define kParamShadowXHint "Shadow offset X"
#define kParamShadowXDefault 0

#define kParamShadowY "shadowY"
#define kParamShadowYLabel "Offset Y"
#define kParamShadowYHint "Shadow offset Y"
#define kParamShadowYDefault 0

#define kParamShadowBlur "shadowSoften"
#define kParamShadowBlurLabel "Soften"
#define kParamShadowBlurHint "Soften shadow"
#define kParamShadowBlurDefault 0

#define CLAMP(value, min, max) (((value) >(max)) ? (max) : (((value) <(min)) ? (min) : (value)))

using namespace OFX;
static bool gHostIsNatron = false;

bool stringCompare(const std::string & l, const std::string & r) {
    return (l==r);
}

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
    OFX::Clip *srcClip_;
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
    OFX::ChoiceParam *align_;
    OFX::DoubleParam *fontSize_;
    OFX::ChoiceParam *fontName_;
    OFX::StringParam *font_;
    OFX::DoubleParam *shadowOpacity_;
    OFX::DoubleParam *shadowSigma_;
    OFX::RGBParam *shadowColor_;
    OFX::IntParam *shadowX_;
    OFX::IntParam *shadowY_;
    OFX::DoubleParam *shadowBlur_;
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
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(!srcClip_ || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB);

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
    align_ = fetchChoiceParam(kParamAlign);
    fontSize_ = fetchDoubleParam(kParamFontSize);
    fontName_ = fetchChoiceParam(kParamFontName);
    font_ = fetchStringParam(kParamFont);
    shadowOpacity_ = fetchDoubleParam(kParamShadowOpacity);
    shadowSigma_ = fetchDoubleParam(kParamShadowSigma);
    shadowColor_ = fetchRGBParam(kParamShadowColor);
    shadowX_ = fetchIntParam(kParamShadowX);
    shadowY_ = fetchIntParam(kParamShadowY);
    shadowBlur_ = fetchDoubleParam(kParamShadowBlur);

    assert(text_ && width_ && height_ /*&& gravity_*/ && hinting_ && indent_ && justify_ && wrap_ /*&& ellipsize_*/ && single_ && align_ && fontSize_ && fontName_ && font_ && shadowBlur_ && shadowColor_ && shadowOpacity_ && shadowSigma_ && shadowX_ && shadowY_);

    // Setup selected font
    std::string fontString, fontCombo;
    font_->getValue(fontString);
    int fontID;
    int fontCount = fontName_->getNOptions();
    fontName_->getValue(fontID);
    fontName_->getOption(fontID,fontCombo);
    if (!fontString.empty()) {
        if (std::strcmp(fontCombo.c_str(),fontString.c_str())!=0) {
            for(int x = 0; x < fontCount; x++) {
                std::string fontFound;
                fontName_->getOption(x,fontFound);
                if (!fontFound.empty()) {
                    if (std::strcmp(fontFound.c_str(),fontString.c_str())==0) {
                        fontName_->setValue(x);
                        break;
                    }
                }
            }
        }
    }
    else {
        if (!fontCombo.empty())
            font_->setValue(fontCombo);
    }
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
    std::string text, fontName, font;
    int /*gravity,*/ hinting, indent, wrap/*, ellipsize*/, cwidth, cheight, align, fontID, shadowX, shadowY;
    double fontSize, shadowOpacity, shadowSigma, shadowR, shadowG, shadowB, shadowBlur;
    bool justify = false;
    bool single = false;
    text_->getValueAtTime(args.time, text);
    //gravity_->getValueAtTime(args.time, gravity);
    hinting_->getValueAtTime(args.time, hinting);
    indent_->getValueAtTime(args.time, indent);
    justify_->getValueAtTime(args.time, justify);
    wrap_->getValueAtTime(args.time, wrap);
    //ellipsize_->getValueAtTime(args.time, ellipsize);
    width_->getValueAtTime(args.time, cwidth);
    height_->getValueAtTime(args.time, cheight);
    align_->getValueAtTime(args.time, align);
    fontSize_->getValueAtTime(args.time, fontSize);
    fontName_->getValueAtTime(args.time, fontID);
    font_->getValueAtTime(args.time, font);
    fontName_->getOption(fontID,fontName);
    shadowOpacity_->getValueAtTime(args.time, shadowOpacity);
    shadowSigma_->getValueAtTime(args.time, shadowSigma);
    shadowColor_->getValueAtTime(args.time, shadowR, shadowG, shadowB);
    shadowX_->getValueAtTime(args.time, shadowX);
    shadowY_->getValueAtTime(args.time, shadowY);
    shadowBlur_->getValueAtTime(args.time, shadowBlur);

    int shadowRI = ((uint8_t)(255.0f *CLAMP(shadowR, 0.0, 1.0)));
    int shadowGI = ((uint8_t)(255.0f *CLAMP(shadowG, 0.0, 1.0)));
    int shadowBI = ((uint8_t)(255.0f *CLAMP(shadowB, 0.0, 1.0)));
    std::ostringstream shadowRGB;
    shadowRGB << "rgb(" << shadowRI <<"," << shadowGI << "," << shadowBI << ")";

    // always prefer font
    if (!font.empty())
        fontName=font;

    // cascade menu
    if (gHostIsNatron)
        fontName.erase(0,2);

    // no fonts?
    if (fontName.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No fonts found, please check installation");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    Magick::ResourceLimits::thread(1);

    // Generate empty image
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,1)"));

    // set default font and size
    if (!font.empty())
        image.font(fontName);
    image.fontPointsize(std::floor(fontSize * args.renderScale.x + 0.5));

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
    switch(align) { // requires im patch
    case 0: // left
        image.defineValue("pango","align","left");
        break;
    case 1: // right
        image.defineValue("pango","align","right");
        break;
    case 2: // center
        image.defineValue("pango","align","center");
        break;
    }
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

    // Shadow
    if (shadowOpacity>0 && shadowSigma>0) {
        Magick::Image shadowContainer(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
        Magick::Image dropShadow;
        dropShadow=image;
        dropShadow.backgroundColor(shadowRGB.str());
        dropShadow.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
        try { // workaround for missing colors.xml
            dropShadow.shadow(shadowOpacity,std::floor(shadowSigma * args.renderScale.x + 0.5),0,0);
        }
        catch(Magick::Warning &warning) {
            dropShadow.shadow(shadowOpacity,std::floor(shadowSigma * args.renderScale.x + 0.5),0,0);
            #ifdef DEBUG
            std::cout << warning.what() << std::endl;
            #endif
        }
        if (shadowBlur>0)
            dropShadow.blur(0,std::floor(shadowBlur * args.renderScale.x + 0.5));
        shadowContainer.composite(dropShadow,std::floor(shadowX * args.renderScale.x + 0.5),std::floor(shadowY * args.renderScale.x + 0.5),Magick::OverCompositeOp);
        shadowContainer.composite(image,0,0,Magick::OverCompositeOp);
        image=shadowContainer;
    }

    // Flip image
    image.flip();

    // add src clip if any
    if (srcClip_ && srcClip_->isConnected() && cwidth==0 && cheight==0) {
        std::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
        if (srcImg.get()) {
            Magick::Image input;
            input.read(width,height,"RGB",Magick::FloatPixel,(float*)srcImg->getPixelData());
            input.matte(true);
            input.composite(image,0,0,Magick::OverCompositeOp);
            image=input;
        }
    }

    // return image
    if (dstClip_ && dstClip_->isConnected()) {
        output.composite(image, 0, 0, Magick::OverCompositeOp);
        output.composite(image, 0, 0, Magick::CopyOpacityCompositeOp);
        output.write(0,0,args.renderWindow.x2 - args.renderWindow.x1,args.renderWindow.y2 - args.renderWindow.y1,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
    }
}

void TextPangoPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (paramName == kParamFontName) {
        std::string font;
        int fontID;
        fontName_->getValueAtTime(args.time, fontID);
        fontName_->getOption(fontID,font);
        font_->setValueAtTime(args.time, font);
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
    desc.setPluginDescription("Pango text generator node.\n\nPowered by "+magickString+"\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.");

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
    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

    // there has to be an input clip, even for generators
    ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setOptional(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    GroupParamDescriptor *groupCanvas = desc.defineGroupParam("Canvas");
    groupCanvas->setOpen(false);
    {
        page->addChild(*groupCanvas);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamText);
        param->setLabel(kParamTextLabel);
        param->setHint(kParamTextHint);
        param->setStringType(eStringTypeMultiLine);
        param->setAnimates(true);
        param->setDefault("<span color=\"white\">Enter <span color=\"red\">text</span></span>");
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamWidth);
        param->setLabel(kParamWidthLabel);
        param->setHint(kParamWidthHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamWidthDefault);
        param->setParent(*groupCanvas);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamHeight);
        param->setLabel(kParamHeightLabel);
        param->setHint(kParamHeightHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamHeightDefault);
        param->setParent(*groupCanvas);
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
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamFontName);
        param->setLabel(kParamFontNameLabel);
        param->setHint(kParamFontNameHint);
        if (gHostIsNatron)
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);

        // Get all fonts from fontconfig
        int defaultFont = 0;
        int altFont = 0;
        int fontIndex = 0;
        FcConfig* config = FcInitLoadConfigAndFonts();
        FcPattern *p = FcPatternCreate();
        FcObjectSet *os = FcObjectSetBuild (FC_FAMILY,NULL);
        FcFontSet *fs = FcFontList(config, p, os);
        std::list<std::string> fonts;
        for (int i=0; fs && i < fs->nfont; i++) {
            FcPattern *font = fs->fonts[i];
            FcChar8 *s;
            s = FcPatternFormat(font,(const FcChar8 *)"%{family[0]}");
            std::string fontName(reinterpret_cast<char*>(s));
            fonts.push_back(fontName);
            //if (s)
                //free(s);
            if (font)
                FcPatternDestroy(font);
        }
        //if (fs)
            //FcFontSetDestroy(fs);

        // sort fonts
        fonts.sort();
        fonts.erase(unique(fonts.begin(), fonts.end(), stringCompare), fonts.end());

        // add to param
        std::list<std::string>::const_iterator font;
        for(font = fonts.begin(); font != fonts.end(); ++font)
        {
            std::string fontName = *font;
            std::string fontItem;
            if (gHostIsNatron) {
                fontItem=fontName[0];
                fontItem.append("/"+fontName);
            }
            else
                fontItem=fontName;
            param->appendOption(fontItem);
            if (std::strcmp(fontName.c_str(),kParamFontNameDefault)==0)
                defaultFont=fontIndex;
            if (std::strcmp(fontName.c_str(),kParamFontNameAltDefault)==0)
                altFont=fontIndex;
            fontIndex++;
        }

        // set default font
        if (defaultFont>0)
            param->setDefault(defaultFont);
        else if (defaultFont==0&&altFont>0)
            param->setDefault(altFont);

        //
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
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamAlign);
        param->setLabel(kParamAlignLabel);
        param->setHint(kParamAlignHint);
        param->appendOption("Left");
        param->appendOption("Right");
        param->appendOption("Center");
        param->setAnimates(true);
        page->addChild(*param);
    }
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
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamFontSize);
        param->setLabel(kParamFontSizeLabel);
        param->setHint(kParamFontSizeHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamFontSizeDefault);
        param->setAnimates(true);
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
    GroupParamDescriptor *groupShadow = desc.defineGroupParam("Shadow");
    {
        page->addChild(*groupShadow);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamShadowOpacity);
        param->setLabel(kParamShadowOpacityLabel);
        param->setHint(kParamShadowOpacityHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamShadowOpacityDefault);
        param->setParent(*groupShadow);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamShadowSigma);
        param->setLabel(kParamShadowSigmaLabel);
        param->setHint(kParamShadowSigmaHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamShadowSigmaDefault);
        param->setParent(*groupShadow);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamShadowBlur);
        param->setLabel(kParamShadowBlurLabel);
        param->setHint(kParamShadowBlurHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamShadowBlurDefault);
        param->setParent(*groupShadow);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamShadowX);
        param->setLabel(kParamShadowXLabel);
        param->setHint(kParamShadowXHint);
        param->setRange(-10000, 10000);
        param->setDisplayRange(-500, 500);
        param->setDefault(kParamShadowXDefault);
        param->setAnimates(true);
        param->setParent(*groupShadow);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamShadowY);
        param->setLabel(kParamShadowYLabel);
        param->setHint(kParamShadowYHint);
        param->setRange(-10000, 10000);
        param->setDisplayRange(-500, 500);
        param->setDefault(kParamShadowYDefault);
        param->setAnimates(true);
        param->setParent(*groupShadow);
    }
    {
        RGBParamDescriptor* param = desc.defineRGBParam(kParamShadowColor);
        param->setLabel(kParamShadowColorLabel);
        param->setHint(kParamShadowColorHint);
        param->setDefault(0., 0., 0.);
        param->setAnimates(true);
        param->setParent(*groupShadow);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TextPangoPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TextPangoPlugin(handle);
}

static TextPangoPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
