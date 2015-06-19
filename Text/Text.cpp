/*

 openfx-arena - https://github.com/olear/openfx-arena

 Copyright (c) 2015, Ole-André Rodlie <olear@fxarena.net>
 Copyright (c) 2015, FxArena DA <mail@fxarena.net>
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

 * Neither the name of FxArena DA nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


 Based on OIIOText.cpp from openfx-io - https://github.com/MrKepzie/openfx-io

 Written by Alexandre Gauthier.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 Neither the name of the {organization} nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 INRIA
 Domaine de Voluceau
 Rocquencourt - B.P. 105
 78153 Le Chesnay Cedex - France

*/

#include "Text.h"
#include "ofxsPositionInteract.h"
#include "ofxsMacros.h"
#include "ofxNatron.h"
#include <Magick++.h>
#include <sstream>
#include <iostream>
#include <stdint.h>
#include <cmath>
#include <cstring>

#define CLAMP(value, min, max) (((value) >(max)) ? (max) : (((value) <(min)) ? (min) : (value)))

#define kPluginName "Text"
#define kPluginGrouping "Draw"
#define kPluginIdentifier "net.fxarena.openfx.Text"
#define kPluginVersionMajor 4
#define kPluginVersionMinor 5

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

#define kParamPosition "position"
#define kParamPositionLabel "Position"
#define kParamPositionHint "The position of the first character on the first line."

#define kParamInteractive "interactive"
#define kParamInteractiveLabel "Interactive"
#define kParamInteractiveHint "When checked the image will be rendered whenever moving the overlay interact instead of when releasing the mouse button."

#define kParamText "text"
#define kParamTextLabel "Text"
#define kParamTextHint "The text that will be drawn"

#define kParamFontSize "fontSize"
#define kParamFontSizeLabel "Font size"
#define kParamFontSizeHint "The height of the characters to render in pixels"
#define kParamFontSizeDefault 64

#define kParamFontName "fontName"
#define kParamFontNameLabel "Font family"
#define kParamFontNameHint "The name of the font to be used"
#define kParamFontNameDefault "Arial"
#define kParamFontNameAltDefault "DejaVu-Sans" // failsafe on Linux/BSD

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

#define kParamFontOverride "customFont"
#define kParamFontOverrideLabel "Custom font"
#define kParamFontOverrideHint "Override the font list. You can use font name, filename or direct path"

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
    OFX::BooleanParam *use_pango_;
    OFX::RGBParam *shadowColor_;
    OFX::IntParam *shadowX_;
    OFX::IntParam *shadowY_;
    OFX::DoubleParam *shadowBlur_;
    OFX::IntParam *width_;
    OFX::IntParam *height_;
    bool has_fontconfig;
    bool has_freetype;
};

TextPlugin::TextPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, width_(0)
, height_(0)
{
    Magick::InitializeMagick(NULL);

    has_fontconfig = false;
    has_freetype = false;

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("fontconfig") != std::string::npos)
        has_fontconfig = true;
    if (delegates.find("freetype") != std::string::npos)
        has_freetype = true;

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

    assert(position_ && text_ && fontSize_ && fontName_ && textColor_ && strokeColor_ && strokeWidth_ && fontOverride_ && shadowOpacity_ && shadowSigma_ && interlineSpacing_ && interwordSpacing_ && textSpacing_ && shadowColor_ && shadowX_ && shadowY_ && shadowBlur_ && width_ && height_);
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
    int fontSize, fontID, shadowX, shadowY;
    std::string text, fontOverride, fontName;

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
    fontName_->getOption(fontID,fontName);

    // cascade menu
    if (gHostIsNatron)
        fontName.erase(0,2);

    // use custom font
    if (!fontOverride.empty())
        fontName=fontOverride;

    // Generate empty image
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));

    // Set font size
    if (fontSize>0)
        image.fontPointsize(std::floor(fontSize * args.renderScale.x + 0.5));

    // Set stroke width
    if (strokeWidth>0)
        image.strokeWidth(std::floor(strokeWidth * args.renderScale.x + 0.5));

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

    // Setup draw
    std::list<Magick::Drawable> text_draw_list;
    text_draw_list.push_back(Magick::DrawableFont(fontName));
    text_draw_list.push_back(Magick::DrawableText(xtext, ytext, text));
    text_draw_list.push_back(Magick::DrawableFillColor(textRGBA.str()));
    text_draw_list.push_back(Magick::DrawableTextInterlineSpacing(std::floor(interlineSpacing * args.renderScale.x + 0.5)));
    text_draw_list.push_back(Magick::DrawableTextInterwordSpacing(std::floor(interwordSpacing * args.renderScale.x + 0.5)));
    text_draw_list.push_back(Magick::DrawableTextKerning(std::floor(textSpacing * args.renderScale.x + 0.5)));
    if (strokeWidth>0)
        text_draw_list.push_back(Magick::DrawableStrokeColor(strokeRGBA.str()));

    // Draw
    image.draw(text_draw_list);

    // Shadow
    if (shadowOpacity>0 && shadowSigma>0) {
        Magick::Image shadowContainer(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
        Magick::Image dropShadow;
        dropShadow=image;
        dropShadow.backgroundColor(shadowRGB.str());
        dropShadow.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
        dropShadow.shadow(shadowOpacity,std::floor(shadowSigma * args.renderScale.x + 0.5),0,0);
        if (shadowBlur>0)
            dropShadow.blur(0,std::floor(shadowBlur * args.renderScale.x + 0.5));
        shadowContainer.composite(dropShadow,std::floor(shadowX * args.renderScale.x + 0.5),std::floor(shadowY * args.renderScale.x + 0.5),Magick::OverCompositeOp);
        shadowContainer.composite(image,0,0,Magick::OverCompositeOp);
        image=shadowContainer;
    }

    // Flip image
    image.flip();

    // return image
    if (dstClip_ && dstClip_->isConnected())
        image.write(0,0,width,height,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
}

void TextPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &/*paramName*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
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
    std::string magickV = MagickCore::GetMagickVersion(NULL);
    std::string delegates = MagickCore::GetMagickDelegates();
    desc.setPluginDescription("Text generator for Natron.\n\nWritten by Ole-André Rodlie <olear@fxarena.net>\n\n Powered by "+magickV+"\n\nFeatures: "+delegates);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // add other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setOverlayInteractDescriptor(new PositionOverlayDescriptor<PositionInteractParam>);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TextPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{   
    // natron?
    gHostIsNatron = (OFX::getImageEffectHostDescription()->hostName == kNatronOfxHostName);

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
    GroupParamDescriptor *groupStroke = desc.defineGroupParam("Stroke");
    GroupParamDescriptor *groupShadow = desc.defineGroupParam("Shadow");
    GroupParamDescriptor *groupSpace = desc.defineGroupParam("Spacing");
    GroupParamDescriptor *groupCanvas = desc.defineGroupParam("Canvas");

    bool hostHasNativeOverlayForPosition;
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
            param->setUseHostOverlayHandle(true);
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
        param->setStringType(eStringTypeSingleLine);
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
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamTextColor);
        param->setLabel(kParamTextColorLabel);
        param->setHint(kParamTextColorHint);
        param->setDefault(1., 1., 1., 1.);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamStrokeColor);
        param->setLabel(kParamStrokeColorLabel);
        param->setHint(kParamStrokeColorHint);
        param->setDefault(1., 1., 1., 1.);
        param->setAnimates(true);
        param->setParent(*groupStroke);
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
    {
         page->addChild(*groupStroke);
         page->addChild(*groupShadow);
         page->addChild(*groupSpace);
         page->addChild(*groupCanvas);
     }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TextPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TextPlugin(handle);
}

void getTextPluginID(OFX::PluginFactoryArray &ids)
{
    static TextPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
