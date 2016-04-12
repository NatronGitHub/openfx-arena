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
#define kPluginVersionMinor 0

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
#define kParamStyleLabel "Font style"
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
#define kParamWrapHint "Word wrap"
#define kParamWrapDefault 1

#define kParamAlign "align"
#define kParamAlignLabel "Align"
#define kParamAlignHint "Text align"

#define kParamMarkup "markup"
#define kParamMarkupLabel "Markup"
#define kParamMarkupHint "Pango Text Attribute Markup Language, https://developer.gnome.org/pango/stable/PangoMarkupFormat.html"
#define kParamMarkupDefault false

#define kParamWidth "width"
#define kParamWidthLabel "Width"
#define kParamWidthHint "Set canvas width, default (0) is project format"
#define kParamWidthDefault 0

#define kParamHeight "height"
#define kParamHeightLabel "Height"
#define kParamHeightHint "Set canvas height, default (0) is project format"
#define kParamHeightDefault 0

using namespace OFX;
static bool gHostIsNatron = false;

bool stringCompare(const std::string & l, const std::string & r) {
    return (l==r);
}

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
    OFX::StringParam *text_;
    OFX::IntParam *fontSize_;
    OFX::ChoiceParam *fontName_;
    OFX::RGBAParam *textColor_;
    OFX::IntParam *width_;
    OFX::IntParam *height_;
    OFX::StringParam *font_;
    OFX::BooleanParam *justify_;
    OFX::ChoiceParam *wrap_;
    OFX::ChoiceParam *align_;
    OFX::BooleanParam *markup_;
    OFX::ChoiceParam *style_;
};

TextPlugin::TextPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    text_ = fetchStringParam(kParamText);
    fontSize_ = fetchIntParam(kParamFontSize);
    fontName_ = fetchChoiceParam(kParamFontName);
    textColor_ = fetchRGBAParam(kParamTextColor);
    width_ = fetchIntParam(kParamWidth);
    height_ = fetchIntParam(kParamHeight);
    font_ = fetchStringParam(kParamFont);
    justify_ = fetchBooleanParam(kParamJustify);
    wrap_ = fetchChoiceParam(kParamWrap);
    align_ = fetchChoiceParam(kParamAlign);
    markup_ = fetchBooleanParam(kParamMarkup);
    style_ = fetchChoiceParam(kParamStyle);

    assert(text_ && fontSize_ && fontName_ && textColor_ && width_ && height_ && font_ && wrap_ && justify_ && align_ && markup_ && style_);

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
    double r, g, b, a;
    int fontSize, fontID, cwidth,cheight, wrap, align, style;
    std::string text, fontName, font;
    bool justify;
    bool markup;

    text_->getValueAtTime(args.time, text);
    fontSize_->getValueAtTime(args.time, fontSize);
    fontName_->getValueAtTime(args.time, fontID);
    textColor_->getValueAtTime(args.time, r, g, b, a);
    width_->getValueAtTime(args.time, cwidth);
    height_->getValueAtTime(args.time, cheight);
    font_->getValueAtTime(args.time, font);
    fontName_->getOption(fontID,fontName);
    justify_->getValueAtTime(args.time, justify);
    wrap_->getValueAtTime(args.time, wrap);
    align_->getValueAtTime(args.time, align);
    markup_->getValueAtTime(args.time, markup);
    style_->getValueAtTime(args.time, style);

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

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create (surface);

    layout = pango_cairo_create_layout(cr);

    if (markup)
        pango_layout_set_markup(layout, text.c_str(), -1);
    else
        pango_layout_set_text(layout, text.c_str(), -1);

    desc = pango_font_description_from_string(pangoFont.str().c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

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

    cairo_set_source_rgba(cr, r, g, b, a);
    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);

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

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cdata = NULL;
    pixelData = NULL;
    delete[] pixels;
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

/** @brief The basic describe function, passed a plugin descriptor */
void TextPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
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
void TextPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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
        param->setDefault("Enter text");
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
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamStyle);
        param->setLabel(kParamStyleLabel);
        param->setHint(kParamStyleHint);
        param->appendOption("Normal");
        param->appendOption("Bold");
        param->appendOption("Italic");
        param->setDefault(kParamStyleDefault);
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
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamJustify);
        param->setLabel(kParamJustifyLabel);
        param->setHint(kParamJustifyHint);
        param->setDefault(kParamJustifyDefault);
        param->setAnimates(false);
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
