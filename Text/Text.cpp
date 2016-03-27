/*
# Copyright (c) 2015, Ole-André Rodlie <olear@dracolinux.org>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
*/

#include "ofxsPositionInteract.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include "ofxNatron.h"
#include <Magick++.h>
#include <sstream>
#include <iostream>
#include <stdint.h>
#include <cmath>
#include <cstring>

#define CLAMP(value, min, max) (((value) >(max)) ? (max) : (((value) <(min)) ? (min) : (value)))

#define kPluginName "TextOFX"
#define kPluginGrouping "Draw"
#define kPluginIdentifier "net.fxarena.openfx.Text"
#define kPluginVersionMajor 6
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

#define kParamPosition "position"
#define kParamPositionLabel "Position"
#define kParamPositionHint "The position of the first character on the first line."

#define kParamInteractive "interactive"
#define kParamInteractiveLabel "Interactive"
#define kParamInteractiveHint "When checked the image will be rendered whenever moving the overlay interact instead of when releasing the mouse button."

#define kParamText "text"
#define kParamTextLabel "Text"
#define kParamTextHint "The text that will be drawn"

#define kParamFontSize "size"
#define kParamFontSizeLabel "Font size"
#define kParamFontSizeHint "The height of the characters to render in pixels"
#define kParamFontSizeDefault 64

#define kParamFontName "name"
#define kParamFontNameLabel "Font family"
#define kParamFontNameHint "The name of the font to be used"
#define kParamFontNameDefault "Arial"
#define kParamFontNameAltDefault "DejaVu-Sans" // failsafe on Linux/BSD

#define kParamFont "font"
#define kParamFontLabel "Font"
#define kParamFontHint "Selected font"

#define kParamTextColor "textColor"
#define kParamTextColorLabel "Font color"
#define kParamTextColorHint "The fill color of the text to render"

#define kParamStrokeColor "strokeColor"
#define kParamStrokeColorLabel "Color"
#define kParamStrokeColorHint "The stroke color of the text to render"

#define kParamStroke "stroke"
#define kParamStrokeLabel "Width"
#define kParamStrokeHint "Adjust stroke width"
#define kParamStrokeDefault 0

#define kParamFontOverride "custom"
#define kParamFontOverrideLabel "Custom font"
#define kParamFontOverrideHint "Override font family"

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

#define kParamInterlineSpacing "lineSpacing"
#define kParamInterlineSpacingLabel "Line"
#define kParamInterlineSpacingHint "Spacing between lines"
#define kParamInterlineSpacingDefault 0

#define kParamInterwordSpacing "wordSpacing"
#define kParamInterwordSpacingLabel "Word"
#define kParamInterwordSpacingHint "Spacing between words"
#define kParamInterwordSpacingDefault 0

#define kParamTextSpacing "letterSpacing"
#define kParamTextSpacingLabel "Letter"
#define kParamTextSpacingHint "Spacing between letters"
#define kParamTextSpacingDefault 0

#define kParamWidth "width"
#define kParamWidthLabel "Width"
#define kParamWidthHint "Set canvas width, default (0) is project format"
#define kParamWidthDefault 0

#define kParamHeight "height"
#define kParamHeightLabel "Height"
#define kParamHeightHint "Set canvas height, default (0) is project format"
#define kParamHeightDefault 0

#define kParamShadowBlur "shadowSoften"
#define kParamShadowBlurLabel "Soften"
#define kParamShadowBlurHint "Soften shadow"
#define kParamShadowBlurDefault 0

#define kParamGravity "gravity"
#define kParamGravityLabel "Gravity"
#define kParamGravityHint "Select text gravity"
#define kParamGravityDefault 0

#define kParamWrap "wordWrap"
#define kParamWrapLabel "Word Wrap"
#define kParamWrapHint "Wrap text if larger than width"
#define kParamWrapDefault false

#define kParamPadding "padding"
#define kParamPaddingLabel "Padding"
#define kParamPaddingHint "Padding used for word wrap"
#define kParamPaddingDefault 0

using namespace OFX;
static bool gHostIsNatron = false;

class TextPlugin : public OFX::ImageEffect
{
public:
    TextPlugin(OfxImageEffectHandle handle);
    virtual ~TextPlugin();

    /* Override the render */
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;

    /* override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;

    // override the rod call
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;

private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *srcClip_;
    OFX::Clip *dstClip_;
    OFX::Double2DParam *position_;
    OFX::StringParam *text_;
    OFX::IntParam *fontSize_;
    OFX::ChoiceParam *fontName_;
    OFX::RGBAParam *textColor_;
    OFX::RGBAParam *strokeColor_;
    OFX::DoubleParam *strokeWidth_;
    OFX::StringParam *fontOverride_;
    OFX::DoubleParam *shadowOpacity_;
    OFX::DoubleParam *shadowSigma_;
    OFX::DoubleParam *interlineSpacing_;
    OFX::DoubleParam *interwordSpacing_;
    OFX::DoubleParam *textSpacing_;
    OFX::RGBParam *shadowColor_;
    OFX::IntParam *shadowX_;
    OFX::IntParam *shadowY_;
    OFX::DoubleParam *shadowBlur_;
    OFX::IntParam *width_;
    OFX::IntParam *height_;
    OFX::ChoiceParam *gravity_;
    OFX::StringParam *font_;
    OFX::BooleanParam *wrap_;
    OFX::IntParam *padding_;
    bool has_fontconfig;
    bool has_freetype;
};

TextPlugin::TextPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, srcClip_(0)
, dstClip_(0)
, width_(0)
, height_(0)
,has_fontconfig(false)
,has_freetype(false)
{
    Magick::InitializeMagick(NULL);

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("fontconfig") != std::string::npos)
        has_fontconfig = true;
    if (delegates.find("freetype") != std::string::npos)
        has_freetype = true;

    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(!srcClip_ || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB);

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    position_ = fetchDouble2DParam(kParamPosition);
    text_ = fetchStringParam(kParamText);
    fontSize_ = fetchIntParam(kParamFontSize);
    fontName_ = fetchChoiceParam(kParamFontName);
    textColor_ = fetchRGBAParam(kParamTextColor);
    strokeColor_ = fetchRGBAParam(kParamStrokeColor);
    strokeWidth_ = fetchDoubleParam(kParamStroke);
    fontOverride_ = fetchStringParam(kParamFontOverride);
    shadowOpacity_ = fetchDoubleParam(kParamShadowOpacity);
    shadowSigma_ = fetchDoubleParam(kParamShadowSigma);
    interlineSpacing_ = fetchDoubleParam(kParamInterlineSpacing);
    interwordSpacing_ = fetchDoubleParam(kParamInterwordSpacing);
    textSpacing_ = fetchDoubleParam(kParamTextSpacing);
    shadowColor_ = fetchRGBParam(kParamShadowColor);
    shadowX_ = fetchIntParam(kParamShadowX);
    shadowY_ = fetchIntParam(kParamShadowY);
    shadowBlur_ = fetchDoubleParam(kParamShadowBlur);
    width_ = fetchIntParam(kParamWidth);
    height_ = fetchIntParam(kParamHeight);
    gravity_ = fetchChoiceParam(kParamGravity);
    font_ = fetchStringParam(kParamFont);
    wrap_ = fetchBooleanParam(kParamWrap);
    padding_ = fetchIntParam(kParamPadding);

    assert(position_ && text_ && fontSize_ && fontName_ && textColor_ && strokeColor_ && strokeWidth_ && fontOverride_ && shadowOpacity_ && shadowSigma_ && interlineSpacing_ && interwordSpacing_ && textSpacing_ && shadowColor_ && shadowX_ && shadowY_ && shadowBlur_ && width_ && height_ && gravity_ && font_ && wrap_ && padding_);

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

TextPlugin::~TextPlugin()
{
}

/* Override the render */
void TextPlugin::render(const OFX::RenderArguments &args)
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
    if (!has_fontconfig||!has_freetype) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Fontconfig and/or Freetype missing");
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
    double x, y, r, g, b, a, r_s, g_s, b_s, a_s, strokeWidth, shadowOpacity, shadowSigma, interlineSpacing, interwordSpacing, textSpacing, shadowR, shadowG, shadowB, shadowBlur;
    int fontSize, fontID, shadowX, shadowY, gravity,cwidth,cheight,padding;
    std::string text, fontOverride, fontName, font;
    bool wrap;

    position_->getValueAtTime(args.time, x, y);
    text_->getValueAtTime(args.time, text);
    fontSize_->getValueAtTime(args.time, fontSize);
    fontName_->getValueAtTime(args.time, fontID);
    strokeWidth_->getValueAtTime(args.time, strokeWidth);
    fontOverride_->getValueAtTime(args.time, fontOverride);
    textColor_->getValueAtTime(args.time, r, g, b, a);
    strokeColor_->getValueAtTime(args.time, r_s, g_s, b_s, a_s);
    shadowOpacity_->getValueAtTime(args.time, shadowOpacity);
    shadowSigma_->getValueAtTime(args.time, shadowSigma);
    interlineSpacing_->getValueAtTime(args.time, interlineSpacing);
    interwordSpacing_->getValueAtTime(args.time, interwordSpacing);
    textSpacing_->getValueAtTime(args.time, textSpacing);
    shadowColor_->getValueAtTime(args.time, shadowR, shadowG, shadowB);
    shadowX_->getValueAtTime(args.time, shadowX);
    shadowY_->getValueAtTime(args.time, shadowY);
    shadowBlur_->getValueAtTime(args.time, shadowBlur);
    gravity_->getValueAtTime(args.time, gravity);
    width_->getValueAtTime(args.time, cwidth);
    height_->getValueAtTime(args.time, cheight);
    font_->getValueAtTime(args.time, font);
    fontName_->getOption(fontID,fontName);
    wrap_->getValueAtTime(args.time, wrap);
    padding_->getValueAtTime(args.time, padding);

    // always prefer font
    if (!font.empty())
        fontName=font;

    // cascade menu
    if (gHostIsNatron)
        fontName.erase(0,2);

    // use custom font
    if (!fontOverride.empty())
        fontName=fontOverride;

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0)
        Magick::ResourceLimits::thread(threads);

    // Generate empty image
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,1)"));

    // no fonts?
    if (fontName.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No fonts found, please check installation");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // Convert colors to int
    int rI = ((uint8_t)(255.0f *CLAMP(r, 0.0, 1.0)));
    int gI = ((uint8_t)(255.0f *CLAMP(g, 0.0, 1.0)));
    int bI = ((uint8_t)(255.0f *CLAMP(b, 0.0, 1.0)));

    int r_sI = ((uint8_t)(255.0f *CLAMP(r_s, 0.0, 1.0)));
    int g_sI = ((uint8_t)(255.0f *CLAMP(g_s, 0.0, 1.0)));
    int b_sI = ((uint8_t)(255.0f *CLAMP(b_s, 0.0, 1.0)));

    int shadowRI = ((uint8_t)(255.0f *CLAMP(shadowR, 0.0, 1.0)));
    int shadowGI = ((uint8_t)(255.0f *CLAMP(shadowG, 0.0, 1.0)));
    int shadowBI = ((uint8_t)(255.0f *CLAMP(shadowB, 0.0, 1.0)));

    std::ostringstream textRGBA;
    textRGBA << "rgba(" << rI <<"," << gI << "," << bI << "," << a << ")";

    std::ostringstream strokeRGBA;
    strokeRGBA << "rgba(" << r_sI <<"," << g_sI << "," << b_sI << "," << a_s << ")";

    std::ostringstream shadowRGB;
    shadowRGB << "rgb(" << shadowRI <<"," << shadowGI << "," << shadowBI << ")";

    // Flip image
    image.flip();

    // Position x y
    double ytext = y*args.renderScale.y;
    double xtext = x*args.renderScale.x;
    int tmp_y = dstRod.y2 - dstBounds.y2;
    int tmp_height = dstBounds.y2 - dstBounds.y1;
    ytext = tmp_y + ((tmp_y+tmp_height-1) - ytext);

    // Setup text draw
    std::list<Magick::Drawable> draw;
    switch(gravity) {
    case 1:
        xtext = xtext-(width/2);
        ytext = ytext-(height/2);
        draw.push_back(Magick::DrawableGravity(Magick::CenterGravity));
        break;
    case 2:
        xtext = 0;
        ytext = 0;
        draw.push_back(Magick::DrawableGravity(Magick::CenterGravity));
        break;
    default:
        //
        break;
    }

    image.fontFamily(fontName);
    image.fontPointsize(std::floor(fontSize * args.renderScale.x + 0.5));

    bool multiline = false;
    if (text.find("\n") != std::string::npos)
        multiline = true;

    Magick::TypeMetric metrics;
    if (multiline)
        image.fontTypeMetricsMultiline(text,&metrics);
    else
        image.fontTypeMetrics(text,&metrics);

    int textWidth = (int)metrics.textWidth()+padding;
    if (textWidth>width && wrap) { // wrap it!
        size_t space = text.find_last_of(" ");
        text.replace(space,1,"\n");
    }

    draw.push_back(Magick::DrawableText(xtext, ytext, text));
    draw.push_back(Magick::DrawableFillColor(Magick::Color(textRGBA.str())));

    draw.push_back(Magick::DrawableTextInterlineSpacing(interlineSpacing * args.renderScale.x));
    draw.push_back(Magick::DrawableTextInterwordSpacing(interwordSpacing * args.renderScale.x));
    draw.push_back(Magick::DrawableTextKerning(textSpacing * args.renderScale.x));

    if (strokeWidth>0) {
        draw.push_back(Magick::DrawableStrokeColor(Magick::Color(strokeRGBA.str())));
        draw.push_back(Magick::DrawableStrokeWidth(std::floor(strokeWidth * args.renderScale.x + 0.5)));
    }

    // Draw
    image.draw(draw);

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

void TextPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
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

bool TextPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(TextPluginFactory, {}, {});

namespace {
struct PositionInteractParam {
    static const char *name() { return kParamPosition; }
    static const char *interactiveName() { return kParamInteractive; }
};
}

/** @brief The basic describe function, passed a plugin descriptor */
void TextPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    desc.setPluginDescription("Text generator node.\n\nPowered by "+magickString+"\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.");

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
    desc.setOverlayInteractDescriptor(new PositionOverlayDescriptor<PositionInteractParam>);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TextPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // natron?
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
    bool hostHasNativeOverlayForPosition;
    {
        page->addChild(*groupCanvas);
    }
    {
        Double2DParamDescriptor* param = desc.defineDouble2DParam(kParamPosition);
        param->setLabel(kParamPositionLabel);
        param->setHint(kParamPositionHint);
        param->setDoubleType(eDoubleTypeXYAbsolute);
        param->setDefaultCoordinateSystem(eCoordinatesNormalised);
        param->setDefault(0.5, 0.5);
        param->setAnimates(true);
        hostHasNativeOverlayForPosition = param->getHostHasNativeOverlayHandle();
        if (hostHasNativeOverlayForPosition)
            param->setUseHostNativeOverlayHandle(true);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamInteractive);
        param->setLabel(kParamInteractiveLabel);
        param->setHint(kParamInteractiveHint);
        param->setAnimates(false);
        page->addChild(*param);

        //Do not show this parameter if the host handles the interact
        if (hostHasNativeOverlayForPosition)
            param->setIsSecret(true);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamGravity);
        param->setLabel(kParamGravityLabel);
        param->setHint(kParamGravityHint);
        param->appendOption("None");
        param->appendOption("Center");
        param->appendOption("Center forced");
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamWrap);
        param->setLabel(kParamWrapLabel);
        param->setHint(kParamWrapHint);
        param->setAnimates(false);
        param->setDefault(kParamWrapDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamPadding);
        param->setLabel(kParamPaddingLabel);
        param->setHint(kParamPaddingHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 1000);
        param->setDefault(kParamPaddingDefault);
        param->setAnimates(false);
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
        StringParamDescriptor* param = desc.defineStringParam(kParamFontOverride);
        param->setLabel(kParamFontOverrideLabel);
        param->setHint(kParamFontOverrideHint);
        param->setStringType(eStringTypeFilePath);
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
        page->addChild(*param);
    }
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamTextColor);
        param->setLabel(kParamTextColorLabel);
        param->setHint(kParamTextColorHint);
        param->setDefault(1., 1., 1., 1.);
        param->setAnimates(true);
        page->addChild(*param);
    }
    GroupParamDescriptor *groupStroke = desc.defineGroupParam("Stroke");
    {
         page->addChild(*groupStroke);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamStroke);
        param->setLabel(kParamStrokeLabel);
        param->setHint(kParamStrokeHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamStrokeDefault);
        param->setParent(*groupStroke);
    }
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamStrokeColor);
        param->setLabel(kParamStrokeColorLabel);
        param->setHint(kParamStrokeColorHint);
        param->setDefault(1., 1., 1., 1.);
        param->setAnimates(true);
        param->setParent(*groupStroke);
    }
    GroupParamDescriptor *groupSpace = desc.defineGroupParam("Spacing");
    {
        page->addChild(*groupSpace);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamTextSpacing);
        param->setLabel(kParamTextSpacingLabel);
        param->setHint(kParamTextSpacingHint);
        param->setRange(-10000, 10000);
        param->setDisplayRange(-500, 500);
        param->setDefault(kParamTextSpacingDefault);
        param->setParent(*groupSpace);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamInterwordSpacing);
        param->setLabel(kParamInterwordSpacingLabel);
        param->setHint(kParamInterwordSpacingHint);
        param->setRange(-10000, 10000);
        param->setDisplayRange(-500, 500);
        param->setDefault(kParamInterwordSpacingDefault);
        param->setParent(*groupSpace);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamInterlineSpacing);
        param->setLabel(kParamInterlineSpacingLabel);
        param->setHint(kParamInterlineSpacingHint);
        param->setRange(-10000, 10000);
        param->setDisplayRange(-500, 500);
        param->setDefault(kParamInterlineSpacingDefault);
        param->setParent(*groupSpace);
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
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TextPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TextPlugin(handle);
}

static TextPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
