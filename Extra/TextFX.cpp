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

#include <pango/pangocairo.h>
#include <fontconfig/fontconfig.h>

#include "ofxsMacros.h"
#include "ofxsImageEffect.h"
#include "ofxNatron.h"

#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>

#define kPluginName "TextFX"
#define kPluginGrouping "Draw"
#define kPluginIdentifier "fr.inria.openfx.TextFX"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 5

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe

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
#define kParamFontNameAltDefault "DejaVu Sans" // failsafe on Linux/BSD

#define kParamFont "font"
#define kParamFontLabel "Font"
#define kParamFontHint "Selected font"

#define kParamStyle "style"
#define kParamStyleLabel "Style"
#define kParamStyleHint "Font style"
#define kParamStyleDefault 0

#define kParamTextColor "color"
#define kParamTextColorLabel "Font color"
#define kParamTextColorHint "The fill color of the text to render"

#define kParamJustify "justify"
#define kParamJustifyLabel "Justify"
#define kParamJustifyHint "Text justify"
#define kParamJustifyDefault false

#define kParamWrap "wrap"
#define kParamWrapLabel "Wrap"
#define kParamWrapHint "Word wrap. Disabled if auto size is active"
#define kParamWrapDefault 0

#define kParamAlign "align"
#define kParamAlignLabel "Align"
#define kParamAlignHint "Text align"
#define kParamAlignDefault 0

#define kParamMarkup "markup"
#define kParamMarkupLabel "Markup"
#define kParamMarkupHint "Pango Text Attribute Markup Language, https://developer.gnome.org/pango/stable/PangoMarkupFormat.html"
#define kParamMarkupDefault false

#define kParamAutoSize "autoSize"
#define kParamAutoSizeLabel "Auto size"
#define kParamAutoSizeHint "Set canvas sized based on text. This will disable word wrap, custom canvas size and circle effect."
#define kParamAutoSizeDefault true

#define kParamStretch "stretch"
#define kParamStretchLabel "Stretch"
#define kParamStretchHint "Width of the font relative to other designs within a family"
#define kParamStretchDefault 4

#define kParamWeight "weight"
#define kParamWeightLabel "Weight"
#define kParamWeightHint "The weight field specifies how bold or light the font should be"
#define kParamWeightDefault 5

#define kParamStrokeColor "strokeColor"
#define kParamStrokeColorLabel "Stroke color"
#define kParamStrokeColorHint "The fill color of the stroke to render"

#define kParamStrokeWidth "strokeSize"
#define kParamStrokeWidthLabel "Stroke size"
#define kParamStrokeWidthHint "Stroke size"
#define kParamStrokeWidthDefault 0.0

#define kParamStrokeDash "strokeDash"
#define kParamStrokeDashLabel "Stroke dash length"
#define kParamStrokeDashHint "The length of the dashes."
#define kParamStrokeDashDefault 0

#define kParamStrokeDashPattern "strokeDashPattern"
#define kParamStrokeDashPatternLabel "Stroke dash pattern"
#define kParamStrokeDashPatternHint "An array specifying alternate lengths of on and off stroke portions."

#define kParamFontAA "antialiasing"
#define kParamFontAALabel "Antialiasing"
#define kParamFontAAHint "This specifies the type of antialiasing to do when rendering text."
#define kParamFontAADefault 0

#define kParamSubpixel "subpixel"
#define kParamSubpixelLabel "Subpixel"
#define kParamSubpixelHint " The subpixel order specifies the order of color elements within each pixel on the dets the antialiasing mode for the fontisplay device when rendering with an antialiasing mode."
#define kParamSubpixelDefault 0

#define kParamHintStyle "hintStyle"
#define kParamHintStyleLabel "Hint style"
#define kParamHintStyleHint "This controls whether to fit font outlines to the pixel grid, and if so, whether to optimize for fidelity or contrast."
#define kParamHintStyleDefault 0

#define kParamHintMetrics "hintMetrics"
#define kParamHintMetricsLabel "Hint metrics"
#define kParamHintMetricsHint "This controls whether metrics are quantized to integer values in device units."
#define kParamHintMetricsDefault 0

#define kParamLetterSpace "letterSpace"
#define kParamLetterSpaceLabel "Letter spacing"
#define kParamLetterSpaceHint "Spacing between letters"
#define kParamLetterSpaceDefault 0

#define kParamCircleRadius "circleRadius"
#define kParamCircleRadiusLabel "Circle radius"
#define kParamCircleRadiusHint "Circle radius. Effect only works if auto size is disabled."
#define kParamCircleRadiusDefault 0

#define kParamCircleWords "circleWords"
#define kParamCircleWordsLabel "Circle Words"
#define kParamCircleWordsHint "X times text in circle"
#define kParamCircleWordsDefault 10

#define kParamCanvas "canvas"
#define kParamCanvasLabel "Canvas size"
#define kParamCanvasHint "Set canvas size, default (0) is project format. Disabled if auto size is active"
#define kParamCanvasDefault 0

using namespace OFX;
static bool gHostIsNatron = false;

bool stringCompare(const std::string & l, const std::string & r) {
    return (l==r);
}

class TextFXPlugin : public OFX::ImageEffect
{
public:
    TextFXPlugin(OfxImageEffectHandle handle);
    virtual ~TextFXPlugin();

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
    OFX::IntParam *fontSize_;
    OFX::ChoiceParam *fontName_;
    OFX::RGBAParam *textColor_;
    OFX::StringParam *font_;
    OFX::BooleanParam *justify_;
    OFX::ChoiceParam *wrap_;
    OFX::ChoiceParam *align_;
    OFX::BooleanParam *markup_;
    OFX::ChoiceParam *style_;
    OFX::BooleanParam *auto_;
    OFX::ChoiceParam *stretch_;
    OFX::ChoiceParam *weight_;
    OFX::RGBAParam *strokeColor_;
    OFX::DoubleParam *strokeWidth_;
    OFX::IntParam *strokeDash_;
    OFX::Double3DParam *strokeDashPattern_;
    OFX::ChoiceParam *fontAA_;
    OFX::ChoiceParam *subpixel_;
    OFX::ChoiceParam *hintStyle_;
    OFX::ChoiceParam *hintMetrics_;
    OFX::DoubleParam *circleRadius_;
    OFX::IntParam *circleWords_;
    OFX::IntParam *letterSpace_;
    OFX::Int2DParam *canvas_;
};

TextFXPlugin::TextFXPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    text_ = fetchStringParam(kParamText);
    fontSize_ = fetchIntParam(kParamFontSize);
    fontName_ = fetchChoiceParam(kParamFontName);
    textColor_ = fetchRGBAParam(kParamTextColor);
    font_ = fetchStringParam(kParamFont);
    justify_ = fetchBooleanParam(kParamJustify);
    wrap_ = fetchChoiceParam(kParamWrap);
    align_ = fetchChoiceParam(kParamAlign);
    markup_ = fetchBooleanParam(kParamMarkup);
    style_ = fetchChoiceParam(kParamStyle);
    auto_ = fetchBooleanParam(kParamAutoSize);
    stretch_ = fetchChoiceParam(kParamStretch);
    weight_ = fetchChoiceParam(kParamWeight);
    strokeColor_ = fetchRGBAParam(kParamStrokeColor);
    strokeWidth_ = fetchDoubleParam(kParamStrokeWidth);
    strokeDash_ = fetchIntParam(kParamStrokeDash);
    strokeDashPattern_ = fetchDouble3DParam(kParamStrokeDashPattern);
    fontAA_ = fetchChoiceParam(kParamFontAA);
    subpixel_ = fetchChoiceParam(kParamSubpixel);
    hintStyle_ = fetchChoiceParam(kParamHintStyle);
    hintMetrics_ = fetchChoiceParam(kParamHintMetrics);
    circleRadius_ = fetchDoubleParam(kParamCircleRadius);
    circleWords_ = fetchIntParam(kParamCircleWords);
    letterSpace_ = fetchIntParam(kParamLetterSpace);
    canvas_ = fetchInt2DParam(kParamCanvas);

    assert(text_ && fontSize_ && fontName_ && textColor_ && font_ && wrap_
           && justify_ && align_ && markup_ && style_ && auto_ && stretch_ && weight_ && strokeColor_
           && strokeWidth_ && strokeDash_ && strokeDashPattern_ && fontAA_ && subpixel_ && hintStyle_
           && hintMetrics_ && circleRadius_ && circleWords_ && letterSpace_ && canvas_);

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


TextFXPlugin::~TextFXPlugin()
{
}

/* Override the render */
void TextFXPlugin::render(const OFX::RenderArguments &args)
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
    double r, g, b, a, s_r, s_g, s_b, s_a, strokeWidth, strokeDashX, strokeDashY, strokeDashZ, circleRadius;
    int fontSize, fontID, cwidth, cheight, wrap, align, style, stretch, weight, strokeDash, fontAA, subpixel, hintStyle, hintMetrics, circleWords, letterSpace;
    std::string text, fontName, font;
    bool justify;
    bool markup;
    bool autoSize;

    text_->getValueAtTime(args.time, text);
    fontSize_->getValueAtTime(args.time, fontSize);
    fontName_->getValueAtTime(args.time, fontID);
    textColor_->getValueAtTime(args.time, r, g, b, a);
    font_->getValueAtTime(args.time, font);
    fontName_->getOption(fontID,fontName);
    justify_->getValueAtTime(args.time, justify);
    wrap_->getValueAtTime(args.time, wrap);
    align_->getValueAtTime(args.time, align);
    markup_->getValueAtTime(args.time, markup);
    style_->getValueAtTime(args.time, style);
    auto_->getValueAtTime(args.time, autoSize);
    stretch_->getValueAtTime(args.time, stretch);
    weight_->getValueAtTime(args.time, weight);
    strokeColor_->getValueAtTime(args.time, s_r, s_g, s_b, s_a);
    strokeWidth_->getValueAtTime(args.time, strokeWidth);
    strokeDash_->getValueAtTime(args.time, strokeDash);
    strokeDashPattern_->getValueAtTime(args.time, strokeDashX, strokeDashY, strokeDashZ);
    fontAA_->getValueAtTime(args.time, fontAA);
    subpixel_->getValueAtTime(args.time, subpixel);
    hintStyle_->getValueAtTime(args.time, hintStyle);
    hintMetrics_->getValueAtTime(args.time, hintMetrics);
    circleRadius_->getValueAtTime(args.time, circleRadius);
    circleWords_->getValueAtTime(args.time, circleWords);
    letterSpace_->getValueAtTime(args.time, letterSpace);
    canvas_->getValueAtTime(args.time, cwidth, cheight);

    if (!font.empty())
        fontName=font;
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "No fonts found/selected");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    if (gHostIsNatron)
        fontName.erase(0,2);

    std::ostringstream pangoFont;
    pangoFont << fontName;
    switch(style) {
    case 0:
        pangoFont << " " << "normal";
        break;
    case 1:
        pangoFont << " " << "bold";
        break;
    case 2:
        pangoFont << " " << "italic";
        break;
    }
    pangoFont << " " << std::floor(fontSize * args.renderScale.x + 0.5);

    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;

    cairo_t *cr;
    cairo_status_t status;
    cairo_surface_t *surface;
    PangoLayout *layout;
    PangoFontDescription *desc;
    PangoAttrList *alist;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create (surface);

    layout = pango_cairo_create_layout(cr);
    alist = pango_attr_list_new();

    cairo_font_options_t* options = cairo_font_options_create();

    switch(hintStyle) {
    case 0:
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_DEFAULT);
        break;
    case 1:
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
        break;
    case 2:
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_SLIGHT);
        break;
    case 3:
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_MEDIUM);
        break;
    case 4:
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_FULL);
        break;
    }

    switch(hintMetrics) {
    case 0:
        cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_DEFAULT);
        break;
    case 1:
        cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
        break;
    case 2:
        cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_ON);
        break;
    }

    switch(fontAA) {
    case 0:
        cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_DEFAULT);
        break;
    case 1:
        cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_NONE);
        break;
    case 2:
        cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);
        break;
    case 3:
        cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_SUBPIXEL);
        break;
    }

    switch(subpixel) {
    case 0:
        cairo_font_options_set_subpixel_order(options, CAIRO_SUBPIXEL_ORDER_DEFAULT);
        break;
    case 1:
        cairo_font_options_set_subpixel_order(options, CAIRO_SUBPIXEL_ORDER_RGB);
        break;
    case 2:
        cairo_font_options_set_subpixel_order(options, CAIRO_SUBPIXEL_ORDER_BGR);
        break;
    case 3:
        cairo_font_options_set_subpixel_order(options, CAIRO_SUBPIXEL_ORDER_VRGB);
        break;
    case 4:
        cairo_font_options_set_subpixel_order(options, CAIRO_SUBPIXEL_ORDER_VBGR);
        break;
    }

    pango_cairo_context_set_font_options(pango_layout_get_context(layout), options);

    if (markup)
        pango_layout_set_markup(layout, text.c_str(), -1);
    else
        pango_layout_set_text(layout, text.c_str(), -1);

    desc = pango_font_description_from_string(pangoFont.str().c_str());

    switch(stretch) {
    case 0:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_ULTRA_CONDENSED);
        break;
    case 1:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_EXTRA_CONDENSED);
        break;
    case 2:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_CONDENSED);
        break;
    case 3:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_SEMI_CONDENSED);
        break;
    case 4:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_NORMAL);
        break;
    case 5:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_SEMI_EXPANDED);
        break;
    case 6:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_EXPANDED);
        break;
    case 7:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_EXTRA_EXPANDED);
        break;
    case 8:
        pango_font_description_set_stretch(desc, PANGO_STRETCH_ULTRA_EXPANDED);
        break;
    }

    switch(weight) {
    case 0:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_THIN);
        break;
    case 1:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRALIGHT);
        break;
    case 2:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_LIGHT);
        break;
    case 3:
#ifndef LEGACY
        pango_font_description_set_weight(desc, PANGO_WEIGHT_SEMILIGHT);
#endif
        break;
    case 4:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_BOOK);
        break;
    case 5:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
        break;
    case 6:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_MEDIUM);
        break;
    case 7:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_SEMIBOLD);
        break;
    case 8:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
        break;
    case 9:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRABOLD);
        break;
    case 10:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_HEAVY);
        break;
    case 11:
        pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRAHEAVY);
        break;
    }

    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    if (!autoSize) {
        switch(wrap) {
        case 1:
            pango_layout_set_width(layout, width * PANGO_SCALE);
            pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
            break;
        case 2:
            pango_layout_set_width(layout, width * PANGO_SCALE);
            pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
            break;
        case 3:
            pango_layout_set_width(layout, width * PANGO_SCALE);
            pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
            break;
        default:
            pango_layout_set_width(layout, -1);
            break;
        }
    }

    switch(align) {
    case 0:
        pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
        break;
    case 1:
        pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);
        break;
    case 2:
        pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
        break;
    }

    if (justify) {
        pango_layout_set_justify (layout, true);
    }

    if (letterSpace != 0) {
        PangoAttribute *lineAttr;
        lineAttr = pango_attr_letter_spacing_new(std::floor((letterSpace*PANGO_SCALE) * args.renderScale.x + 0.5));
        pango_attr_list_insert(alist,lineAttr);
    }

    pango_layout_set_attributes(layout,alist);

    if (strokeWidth>0) {
        if (circleRadius==0) {
            if (strokeDash>0) {
                if (strokeDashX<0.1)
                    strokeDashX=0.1;
                if (strokeDashY<0)
                    strokeDashY=0;
                if (strokeDashZ<0)
                    strokeDashZ=0;
                double dash[] = {strokeDashX, strokeDashY, strokeDashZ};
                cairo_set_dash(cr, dash, strokeDash, 0);
            }

            cairo_new_path(cr);

            if (autoSize)
                cairo_move_to(cr, std::floor((strokeWidth/2) * args.renderScale.x + 0.5), 0.0);

            pango_cairo_layout_path(cr, layout);
            cairo_set_line_width(cr, std::floor(strokeWidth * args.renderScale.x + 0.5));
            //cairo_set_miter_limit(cr, );
            cairo_set_source_rgba(cr, s_r, s_g, s_b, s_a);
            cairo_stroke_preserve(cr);
            cairo_set_source_rgba(cr, r, g, b, a);
            cairo_fill(cr);
        }
    }
    else {
        if (circleRadius==0) {
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
            // workaround antialias issues on windows
            cairo_new_path(cr);
            pango_cairo_layout_path(cr, layout);
            cairo_set_source_rgba(cr, r, g, b, a);
            cairo_fill(cr);
#else
            cairo_set_source_rgba(cr, r, g, b, a);
            pango_cairo_update_layout(cr, layout);
            pango_cairo_show_layout(cr, layout);
#endif
        }
    }

    if (circleRadius>0 && !autoSize) {
        cairo_translate (cr, width/2, height/2);
        for (int i = 0; i < circleWords; i++) {
            int rwidth, rheight;
            double angle = (360. * i) / circleWords;
            cairo_save (cr);
            cairo_set_source_rgba (cr, r, g, b, a);
            cairo_rotate (cr, angle * G_PI / 180.);
            pango_cairo_update_layout (cr, layout);
            pango_layout_get_size (layout, &rwidth, &rheight);
            cairo_move_to (cr, - ((double)rwidth / PANGO_SCALE) / 2, - std::floor(circleRadius * args.renderScale.x + 0.5));
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
            // workaround antialias issues on windows
            cairo_new_path(cr);
            pango_cairo_layout_path(cr, layout);
            cairo_fill(cr);
#else
            pango_cairo_show_layout (cr, layout);
#endif
            cairo_restore (cr);
        }
    }

    status = cairo_status(cr);

    if (status) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    cairo_surface_flush(surface);

    unsigned char* cdata = cairo_image_surface_get_data(surface);
    unsigned char* pixels = new unsigned char[width * height * 4];
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            for (int k = 0; k < 4; ++k)
                pixels[(i + j * width) * 4 + k] = cdata[(i + (height - 1 - j) * width) * 4 + k];
        }
    }

    float* pixelData = (float*)dstImg->getPixelData();
    int offset = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixelData[offset] = (float)pixels[offset + 2] / 255.f;
            pixelData[offset + 1] = (float)pixels[offset + 1] / 255.f;
            pixelData[offset + 2] = (float)pixels[offset] / 255.f;
            pixelData[offset + 3] = (float)pixels[offset + 3] / 255.f;
            offset += 4;
        }
    }

    pango_attr_list_unref(alist);
    cairo_font_options_destroy(options);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cdata = NULL;
    pixelData = NULL;
    delete[] pixels;
}

void TextFXPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
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

bool TextFXPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }

    int width,height;
    bool autoSize;

    canvas_->getValue(width, height);
    auto_->getValue(autoSize);

    if (autoSize) {
        int fontSize, fontID, style, stretch, weight, letterSpace;
        double strokeWidth;
        std::string text, fontName, font;
        bool markup;

        text_->getValueAtTime(args.time, text);
        fontSize_->getValueAtTime(args.time, fontSize);
        fontName_->getValueAtTime(args.time, fontID);
        font_->getValueAtTime(args.time, font);
        fontName_->getOption(fontID,fontName);
        style_->getValueAtTime(args.time, style);
        markup_->getValueAtTime(args.time, markup);
        stretch_->getValueAtTime(args.time, stretch);
        weight_->getValueAtTime(args.time, weight);
        strokeWidth_->getValueAtTime(args.time, strokeWidth);
        letterSpace_->getValueAtTime(args.time, letterSpace);

        if (!font.empty()) {
            fontName=font;
            if (gHostIsNatron)
                fontName.erase(0,2);

            std::ostringstream pangoFont;
            pangoFont << fontName;
            switch(style) {
            case 0:
                pangoFont << " " << "normal";
                break;
            case 1:
                pangoFont << " " << "bold";
                break;
            case 2:
                pangoFont << " " << "italic";
                break;
            }
            pangoFont << " " << fontSize;

            width = rod.x2-rod.x1;
            height = rod.y2-rod.y1;

            cairo_t *cr;
            cairo_surface_t *surface;
            PangoLayout *layout;
            PangoFontDescription *desc;
            PangoAttrList *alist;

            surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
            cr = cairo_create (surface);

            layout = pango_cairo_create_layout(cr);
            alist = pango_attr_list_new();

            if (markup)
                pango_layout_set_markup(layout, text.c_str(), -1);
            else
                pango_layout_set_text(layout, text.c_str(), -1);

            desc = pango_font_description_from_string(pangoFont.str().c_str());

            switch(stretch) {
            case 0:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_ULTRA_CONDENSED);
                break;
            case 1:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_EXTRA_CONDENSED);
                break;
            case 2:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_CONDENSED);
                break;
            case 3:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_SEMI_CONDENSED);
                break;
            case 4:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_NORMAL);
                break;
            case 5:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_SEMI_EXPANDED);
                break;
            case 6:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_EXPANDED);
                break;
            case 7:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_EXTRA_EXPANDED);
                break;
            case 8:
                pango_font_description_set_stretch(desc, PANGO_STRETCH_ULTRA_EXPANDED);
                break;
            }

            switch(weight) {
            case 0:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_THIN);
                break;
            case 1:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRALIGHT);
                break;
            case 2:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_LIGHT);
                break;
            case 3:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_SEMILIGHT);
                break;
            case 4:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_BOOK);
                break;
            case 5:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);
                break;
            case 6:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_MEDIUM);
                break;
            case 7:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_SEMIBOLD);
                break;
            case 8:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
                break;
            case 9:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRABOLD);
                break;
            case 10:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_HEAVY);
                break;
            case 11:
                pango_font_description_set_weight(desc, PANGO_WEIGHT_ULTRAHEAVY);
                break;
            }

            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);

            if (letterSpace != 0) {
                PangoAttribute *lineAttr;
                lineAttr = pango_attr_letter_spacing_new(letterSpace*PANGO_SCALE);
                pango_attr_list_insert(alist,lineAttr);
            }

            pango_layout_set_attributes(layout,alist);

            pango_layout_get_pixel_size(layout, &width, &height);

            /// WIP
            if (strokeWidth>0) {
                width = width+(strokeWidth*2);
                height = height+(strokeWidth/2);
            }

            pango_attr_list_unref(alist);
            g_object_unref(layout);
            cairo_destroy(cr);
            cairo_surface_destroy(surface);
        }
    }

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

mDeclarePluginFactory(TextFXPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void TextFXPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("Text generator node");

    // add the supported contexts
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // add other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TextFXPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // natron?
    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

    // there has to be an input clip, even for generators
    ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->setOptional(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamAutoSize);
        param->setLabel(kParamAutoSizeLabel);
        param->setHint(kParamAutoSizeHint);
        param->setDefault(kParamAutoSizeDefault);
        param->setAnimates(false);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        page->addChild(*param);
    }
    {
        Int2DParamDescriptor* param = desc.defineInt2DParam(kParamCanvas);
        param->setLabel(kParamCanvasLabel);
        param->setHint(kParamCanvasHint);
        param->setRange(0, 0, 10000, 10000);
        param->setDisplayRange(0, 0, 4000, 4000);
        param->setDefault(kParamCanvasDefault, kParamCanvasDefault);
        param->setAnimates(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamMarkup);
        param->setLabel(kParamMarkupLabel);
        param->setHint(kParamMarkupHint);
        param->setDefault(kParamMarkupDefault);
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
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamJustify);
        param->setLabel(kParamJustifyLabel);
        param->setHint(kParamJustifyHint);
        param->setDefault(kParamJustifyDefault);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamWrap);
        param->setLabel(kParamWrapLabel);
        param->setHint(kParamWrapHint);
        param->appendOption("None");
        param->appendOption("Word");
        param->appendOption("Char");
        param->appendOption("Word-Char");
        param->setDefault(kParamWrapDefault);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamAlign);
        param->setLabel(kParamAlignLabel);
        param->setHint(kParamAlignHint);
        param->appendOption("Left");
        param->appendOption("Right");
        param->appendOption("Center");
        param->setDefault(kParamAlignDefault);
        param->setAnimates(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
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
            if (font)
                FcPatternDestroy(font);
        }

        // sort fonts
        fonts.sort();
        fonts.erase(unique(fonts.begin(), fonts.end(), stringCompare), fonts.end());

        // add to param
        std::list<std::string>::const_iterator font;
        for(font = fonts.begin(); font != fonts.end(); ++font) {
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

        if (defaultFont>0)
            param->setDefault(defaultFont);
        else if (defaultFont==0&&altFont>0)
            param->setDefault(altFont);

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
        param->setRange(1, 10000);
        param->setDisplayRange(1, 500);
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
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamLetterSpace);
        param->setLabel(kParamLetterSpaceLabel);
        param->setHint(kParamLetterSpaceHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamLetterSpaceDefault);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamHintStyle);
        param->setLabel(kParamHintStyleLabel);
        param->setHint(kParamHintStyleHint);
        param->appendOption("Default");
        param->appendOption("None");
        param->appendOption("Slight");
        param->appendOption("Medium");
        param->appendOption("Full");
        param->setDefault(kParamHintStyleDefault);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamHintMetrics);
        param->setLabel(kParamHintMetricsLabel);
        param->setHint(kParamHintMetricsHint);
        param->appendOption("Default");
        param->appendOption("Off");
        param->appendOption("On");
        param->setDefault(kParamHintMetricsDefault);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamFontAA);
        param->setLabel(kParamFontAALabel);
        param->setHint(kParamFontAAHint);
        param->appendOption("Default");
        param->appendOption("None");
        param->appendOption("Gray");
        param->appendOption("Subpixel");
        param->setDefault(kParamFontAADefault);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamSubpixel);
        param->setLabel(kParamSubpixelLabel);
        param->setHint(kParamSubpixelHint);
        param->appendOption("Default");
        param->appendOption("RGB");
        param->appendOption("BGR");
        param->appendOption("VRGB");
        param->appendOption("VBGR");
        param->setDefault(kParamSubpixelDefault);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamStyle);
        param->setLabel(kParamStyleLabel);
        param->setHint(kParamStyleHint);
        param->appendOption("Normal");
        param->appendOption("Bold");
        param->appendOption("Italic");
        param->setDefault(kParamStyleDefault);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamWeight);
        param->setLabel(kParamWeightLabel);
        param->setHint(kParamWeightHint);
        param->appendOption("Thin");
        param->appendOption("Ultra light");
        param->appendOption("Light");
        param->appendOption("Semi light");
        param->appendOption("Book");
        param->appendOption("Normal");
        param->appendOption("Medium");
        param->appendOption("Semi bold");
        param->appendOption("Bold");
        param->appendOption("Ultra bold");
        param->appendOption("Heavy");
        param->appendOption("Ultra heavy");
        param->setDefault(kParamWeightDefault);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamStretch);
        param->setLabel(kParamStretchLabel);
        param->setHint(kParamStretchHint);
        param->appendOption("Ultra condensed");
        param->appendOption("Extra condensed");
        param->appendOption("Condensed");
        param->appendOption("Semi condensed");
        param->appendOption("Normal");
        param->appendOption("Semi expanded");
        param->appendOption("Expanded");
        param->appendOption("Extra expanded");
        param->appendOption("Ultra expanded");
        param->setDefault(kParamStretchDefault);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamStrokeWidth);
        param->setLabel(kParamStrokeWidthLabel);
        param->setHint(kParamStrokeWidthHint);
        param->setRange(0, 500);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamStrokeWidthDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamStrokeColor);
        param->setLabel(kParamStrokeColorLabel);
        param->setHint(kParamStrokeColorHint);
        param->setDefault(1., 0., 0., 1.);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamStrokeDash);
        param->setLabel(kParamStrokeDashLabel);
        param->setHint(kParamStrokeDashHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamStrokeDashDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        Double3DParamDescriptor* param = desc.defineDouble3DParam(kParamStrokeDashPattern);
        param->setLabel(kParamStrokeDashPatternLabel);
        param->setHint(kParamStrokeDashPatternHint);
        param->setDefault(1.0, 0.0, 0.0);
        param->setAnimates(true);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamCircleRadius);
        param->setLabel(kParamCircleRadiusLabel);
        param->setHint(kParamCircleRadiusHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 1000);
        param->setDefault(kParamCircleRadiusDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamCircleWords);
        param->setLabel(kParamCircleWordsLabel);
        param->setHint(kParamCircleWordsHint);
        param->setRange(1, 1000);
        param->setDisplayRange(1, 100);
        param->setDefault(kParamCircleWordsDefault);
        param->setAnimates(true);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TextFXPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TextFXPlugin(handle);
}

static TextFXPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
