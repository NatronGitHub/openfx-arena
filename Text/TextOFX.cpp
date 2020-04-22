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

#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#include <fontconfig/fontconfig.h>

#include "ofxsMacros.h"
#include "ofxsImageEffect.h"
#include "ofxNatron.h"
#include "ofxsTransform3x3.h"
#include "ofxsTransformInteract.h"

#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <fstream>

#include "fx.h"
#include "RichText.h" // common text related functions

#define kPluginName "TextOFX"
#define kPluginGrouping "Draw"
#define kPluginIdentifier "net.fxarena.openfx.Text"
#define kPluginVersionMajor 6
#define kPluginVersionMinor 12

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe

#define kParamText "text"
#define kParamTextLabel "Text"
#define kParamTextHint "The text that will be drawn."

#define kParamFontSize "size"
#define kParamFontSizeLabel "Font size"
#define kParamFontSizeHint "The height of the characters to render in pixels. Should not be used for animation, see the scale param."
#define kParamFontSizeDefault 64

#define kParamFontName "name"
#define kParamFontNameLabel "Font family"
#define kParamFontNameHint "The name of the font to be used."
#define kParamFontNameDefault "Arial"
#define kParamFontNameAltDefault "DejaVu Sans" // failsafe on Linux/BSD

#define kParamFont "font"
#define kParamFontLabel "Font"
#define kParamFontHint "Selected font."

#define kParamStyle "style"
#define kParamStyleLabel "Style"
#define kParamStyleHint "Font style."
#define kParamStyleDefault 0

#define kParamTextColor "color"
#define kParamTextColorLabel "Font color"
#define kParamTextColorHint "The fill color of the text to render."

#define kParamBGColor "backgroundColor"
#define kParamBGColorLabel "Background Color"
#define kParamBGColorHint "The fill color of the background."

#define kParamJustify "justify"
#define kParamJustifyLabel "Justify"
#define kParamJustifyHint "Text justify."
#define kParamJustifyDefault false

#define kParamWrap "wrap"
#define kParamWrapLabel "Wrap"
#define kParamWrapHint "Word wrap. Disabled if auto size and/or custom position is enabled."
#define kParamWrapDefault 0

#define kParamAlign "align"
#define kParamAlignLabel "Horizontal align"
#define kParamAlignHint "Horizontal text align. Custom position and auto size must be disabled and word wrap must be enabled (any option except none) to get anything else than left align."
#define kParamAlignDefault 0

#define kParamVAlign "valign"
#define kParamVAlignLabel "Vertical align"
#define kParamVAlignHint "Vertical text align. Disabled if custom position and/or auto size is enabled."
#define kParamVAlignDefault 0

#define kParamMarkup "markup"
#define kParamMarkupLabel "Markup"
#define kParamMarkupHint "Pango Text Attribute Markup Language, https://developer.gnome.org/pango/stable/PangoMarkupFormat.html . Colors don't work if Circle/Arc effect is used."
#define kParamMarkupDefault false

#define kParamAutoSize "autoSize"
#define kParamAutoSizeLabel "Auto size"
#define kParamAutoSizeHint "Set canvas sized based on text. This will disable word wrap, custom canvas size and circle effect. Transform functions should also not be used in combination with this feature."
#define kParamAutoSizeDefault false

#define kParamStretch "stretch"
#define kParamStretchLabel "Stretch"
#define kParamStretchHint "Width of the font relative to other designs within a family."
#define kParamStretchDefault 4

#define kParamWeight "weight"
#define kParamWeightLabel "Weight"
#define kParamWeightHint "The weight field specifies how bold or light the font should be."
#define kParamWeightDefault 5

#define kParamStrokeColor "strokeColor"
#define kParamStrokeColorLabel "Stroke color"
#define kParamStrokeColorHint "The fill color of the stroke to render."

#define kParamStrokeWidth "strokeSize"
#define kParamStrokeWidthLabel "Stroke size"
#define kParamStrokeWidthHint "Stroke size."
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
#define kParamLetterSpaceHint "Spacing between letters. Disabled if markup is used."
#define kParamLetterSpaceDefault 0

#define kParamCircleRadius "circleRadius"
#define kParamCircleRadiusLabel "Circle radius"
#define kParamCircleRadiusHint "Circle radius. Effect only works if auto size is disabled."
#define kParamCircleRadiusDefault 0

#define kParamCircleWords "circleWords"
#define kParamCircleWordsLabel "Circle Words"
#define kParamCircleWordsHint "X times text in circle."
#define kParamCircleWordsDefault 10

#define kParamCanvas "canvas"
#define kParamCanvasLabel "Canvas size"
#define kParamCanvasHint "Set canvas size, default (0) is project format. Disabled if auto size is active."
#define kParamCanvasDefault 0

#define kParamArcRadius "arcRadius"
#define kParamArcRadiusLabel "Arc Radius"
#define kParamArcRadiusHint "Arc path radius (size of the path). The Arc effect is an experimental feature. Effect only works if auto size is disabled."
#define kParamArcRadiusDefault 100.0

#define kParamArcAngle "arcAngle"
#define kParamArcAngleLabel "Arc Angle"
#define kParamArcAngleHint "Arc Angle, set to 360 for a full circle. The Arc effect is an experimental feature. Effect only works if auto size is disabled."
#define kParamArcAngleDefault 0

#define kParamPositionMove "transform"
#define kParamPositionMoveLabel "Transform"
#define kParamPositionMoveHint "Use transform overlay for text position."
#define kParamPositionMoveDefault true

#define kParamTextFile "file"
#define kParamTextFileLabel "Text File"
#define kParamTextFileHint "Use text from filename."

#define kParamSubtitleFile "subtitle"
#define kParamSubtitleFileLabel "Subtitle File"
#define kParamSubtitleFileHint "Load and animate a subtitle file (SRT)."

#define kParamFPS "fps"
#define kParamFPSLabel "Frame Rate"
#define kParamFPSHint "The frame rate of the project, for use with subtitles."
#define kParamFPSDefault 24.0

#define kParamCenterInteract "centerInteract"
#define kParamCenterInteractLabel "Center Interact"
#define kParamCenterInteractHint "Center the text in the interact."
#define kParamCenterInteractDefault false

#define kParamFontOverride "custom"
#define kParamFontOverrideLabel "Custom font(s)"
#define kParamFontOverrideHint "Add custom font(s) to the font list. This can be a font file or a directory with fonts.\n\nIf you want a portable project copy all used fonts to [Project]/fonts (or similar) and reference them here."

#define kParamScrollX "scrollX"
#define kParamScrollXLabel "Scroll X"
#define kParamScrollXHint "Scroll canvas X. Only works if Transform, AutoSize, Circle and Arc is disabled/not used."
#define kParamScrollXDefault 0

#define kParamScrollY "scrollY"
#define kParamScrollYLabel "Scroll Y"
#define kParamScrollYHint "Scroll canvas Y. Only works if Transform, AutoSize, Circle and Arc is disabled/not used."
#define kParamScrollYDefault 0

#define kParamGeneratorRange "frameRange"
#define kParamGeneratorRangeLabel "Frame Range"
#define kParamGeneratorRangeHint "Time domain."

using namespace OFX;
static bool gHostIsNatron = false;

static
bool stringCompare(const std::string & l, const std::string & r) {
    return (l==r);
}

static
std::list<std::string> _genFonts(OFX::ChoiceParam *fontName, OFX::StringParam *fontOverride, bool fontOverrideDir, FcConfig *fontConfig, bool properMenu, std::string fontNameDefault, std::string fontNameAltDefault)
{
    int defaultFont = 0;
    int altFont = 0;
    int fontIndex = 0;

    if (!fontConfig) {
        fontConfig = FcInitLoadConfigAndFonts();
    }

    if (fontOverride) {
        std::string fontCustom;
        fontOverride->getValue(fontCustom);
        if (!fontCustom.empty()) {
            const FcChar8 * fileCustom = (const FcChar8 *)fontCustom.c_str();
            if (!fontOverrideDir) {
                FcConfigAppFontAddFile(fontConfig,fileCustom);
            } else {
                FcConfigAppFontAddDir(fontConfig, fileCustom);
            }
        }
    }

    FcPattern *p = FcPatternCreate();
    FcObjectSet *os = FcObjectSetBuild (FC_FAMILY,NULL);
    FcFontSet *fs = FcFontList(fontConfig, p, os);
    std::list<std::string> fonts;
    for (int i=0; fs && i < fs->nfont; i++) {
        FcPattern *font = fs->fonts[i];
        FcChar8 *s;
        s = FcPatternFormat(font,(const FcChar8 *)"%{family[0]}");
        std::string fontName(reinterpret_cast<char*>(s));
        fonts.push_back(fontName);
        if (font) {
            FcPatternDestroy(font);
        }
    }

    fonts.sort();
    fonts.erase(unique(fonts.begin(), fonts.end(), stringCompare), fonts.end());

    if (fontName) {
        fontName->resetOptions();
    }
    std::list<std::string>::const_iterator font;
    for(font = fonts.begin(); font != fonts.end(); ++font) {
        std::string fontNameString = *font;
        std::string fontItem;
        if (properMenu) {
            fontItem=fontNameString[0];
            fontItem.append("/" + fontNameString);
        } else {
            fontItem=fontNameString;
        }

        if (fontName) {
            fontName->appendOption(fontItem);
        }
        if (fontNameString == fontNameDefault) {
            defaultFont=fontIndex;
        }
        if (fontNameString == fontNameAltDefault) {
            altFont=fontIndex;
        }

        fontIndex++;
    }

    if (fontName) {
        if (defaultFont > 0) {
            fontName->setDefault(defaultFont);
        } else if (altFont > 0) {
            fontName->setDefault(altFont);
        }
    }

    return fonts;
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

    /* override the time domain action, only for the general context */
    virtual bool getTimeDomain(OfxRangeD &range) OVERRIDE FINAL;

    std::string textFromFile(std::string filename);
    void loadSRT();
    void resetCenter(double time);
    void setFontDesc(int stretch, int weight, PangoFontDescription* desc);

private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *_dstClip;
    OFX::StringParam *_text;
    OFX::IntParam *_fontSize;
    OFX::RGBAParam *_textColor;
    OFX::RGBAParam *_bgColor;
    OFX::StringParam *_font;
    OFX::BooleanParam *_justify;
    OFX::ChoiceParam *_wrap;
    OFX::ChoiceParam *_align;
    OFX::ChoiceParam *_valign;
    OFX::BooleanParam *_markup;
    OFX::ChoiceParam *_style;
    OFX::BooleanParam *auto_;
    OFX::ChoiceParam *stretch_;
    OFX::ChoiceParam *weight_;
    OFX::RGBAParam *strokeColor_;
    OFX::DoubleParam *strokeWidth_;
    OFX::IntParam *strokeDash_;
    OFX::Double3DParam *strokeDashPattern_;
    OFX::ChoiceParam *fontAA_;
    OFX::ChoiceParam *subpixel_;
    OFX::ChoiceParam *_hintStyle;
    OFX::ChoiceParam *_hintMetrics;
    OFX::DoubleParam *_circleRadius;
    OFX::IntParam *_circleWords;
    OFX::IntParam *_letterSpace;
    OFX::Int2DParam *_canvas;
    OFX::DoubleParam *_arcRadius;
    OFX::DoubleParam *_arcAngle;
    OFX::DoubleParam *_rotate;
    OFX::Double2DParam *_scale;
    OFX::Double2DParam *_position;
    OFX::BooleanParam *_move;
    OFX::StringParam *_txt;
    OFX::DoubleParam *_skewX;
    OFX::DoubleParam *_skewY;
    OFX::BooleanParam *_scaleUniform;
    OFX::BooleanParam *_centerInteract;
    OFX::ChoiceParam *_fontName;
    OFX::StringParam *_fontOverride;
    FcConfig* _fcConfig;
    OFX::DoubleParam *_scrollX;
    OFX::DoubleParam *_scrollY;
    OFX::StringParam *_srt;
    OFX::DoubleParam *_fps;
    OFX::Int2DParam  *_range;
};

TextFXPlugin::TextFXPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, _dstClip(NULL)
, _text(NULL)
, _fontSize(NULL)
, _textColor(NULL)
, _bgColor(NULL)
, _font(NULL)
, _justify(NULL)
, _wrap(NULL)
, _align(NULL)
, _valign(NULL)
, _markup(NULL)
, _style(NULL)
, auto_(NULL)
, stretch_(NULL)
, weight_(NULL)
, strokeColor_(NULL)
, strokeWidth_(NULL)
, strokeDash_(NULL)
, strokeDashPattern_(NULL)
, fontAA_(NULL)
, subpixel_(NULL)
, _hintStyle(NULL)
, _hintMetrics(NULL)
, _circleRadius(NULL)
, _circleWords(NULL)
, _letterSpace(NULL)
, _canvas(NULL)
, _arcRadius(NULL)
, _arcAngle(NULL)
, _rotate(NULL)
, _scale(NULL)
, _position(NULL)
, _move(NULL)
, _txt(NULL)
, _skewX(NULL)
, _skewY(NULL)
, _scaleUniform(NULL)
, _centerInteract(NULL)
, _fontName(NULL)
, _fontOverride(NULL)
, _fcConfig(NULL)
, _scrollX(NULL)
, _scrollY(NULL)
, _srt(NULL)
, _fps(NULL)
, _range(NULL)
{
    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    assert(_dstClip && _dstClip->getPixelComponents() == OFX::ePixelComponentRGBA);

    _text = fetchStringParam(kParamText);
    _fontSize = fetchIntParam(kParamFontSize);
    _fontName = fetchChoiceParam(kParamFontName);
    _textColor = fetchRGBAParam(kParamTextColor);
    _bgColor = fetchRGBAParam(kParamBGColor);
    _font = fetchStringParam(kParamFont);
    _justify = fetchBooleanParam(kParamJustify);
    _wrap = fetchChoiceParam(kParamWrap);
    _align = fetchChoiceParam(kParamAlign);
    _valign = fetchChoiceParam(kParamVAlign);
    _markup = fetchBooleanParam(kParamMarkup);
    _style = fetchChoiceParam(kParamStyle);
    auto_ = fetchBooleanParam(kParamAutoSize);
    stretch_ = fetchChoiceParam(kParamStretch);
    weight_ = fetchChoiceParam(kParamWeight);
    strokeColor_ = fetchRGBAParam(kParamStrokeColor);
    strokeWidth_ = fetchDoubleParam(kParamStrokeWidth);
    strokeDash_ = fetchIntParam(kParamStrokeDash);
    strokeDashPattern_ = fetchDouble3DParam(kParamStrokeDashPattern);
    fontAA_ = fetchChoiceParam(kParamFontAA);
    subpixel_ = fetchChoiceParam(kParamSubpixel);
    _hintStyle = fetchChoiceParam(kParamHintStyle);
    _hintMetrics = fetchChoiceParam(kParamHintMetrics);
    _circleRadius = fetchDoubleParam(kParamCircleRadius);
    _circleWords = fetchIntParam(kParamCircleWords);
    _letterSpace = fetchIntParam(kParamLetterSpace);
    _canvas = fetchInt2DParam(kParamCanvas);
    _arcRadius = fetchDoubleParam(kParamArcRadius);
    _arcAngle = fetchDoubleParam(kParamArcAngle);
    _rotate = fetchDoubleParam(kParamTransformRotateOld);
    _scale = fetchDouble2DParam(kParamTransformScaleOld);
    _position = fetchDouble2DParam(kParamTransformCenterOld);
    _move = fetchBooleanParam(kParamPositionMove);
    _txt = fetchStringParam(kParamTextFile);
    _skewX = fetchDoubleParam(kParamTransformSkewXOld);
    _skewY = fetchDoubleParam(kParamTransformSkewYOld);
    _scaleUniform = fetchBooleanParam(kParamTransformScaleUniformOld);
    _centerInteract = fetchBooleanParam(kParamCenterInteract);
    _fontOverride = fetchStringParam(kParamFontOverride);
    _scrollX = fetchDoubleParam(kParamScrollX);
    _scrollY = fetchDoubleParam(kParamScrollY);
    _srt = fetchStringParam(kParamSubtitleFile);
    _fps = fetchDoubleParam(kParamFPS);
    if (getContext() == eContextGeneral) {
        _range   = fetchInt2DParam(kParamGeneratorRange);
        assert(_range);
    }

    assert(_text && _fontSize && _fontName && _textColor && _bgColor && _font && _wrap
           && _justify && _align && _valign && _markup && _style && auto_ && stretch_ && weight_ && strokeColor_
           && strokeWidth_ && strokeDash_ && strokeDashPattern_ && fontAA_ && subpixel_ && _hintStyle
           && _hintMetrics && _circleRadius && _circleWords && _letterSpace && _canvas
           && _arcRadius && _arcAngle && _rotate && _scale && _position && _move && _txt
           && _skewX && _skewY && _scaleUniform && _centerInteract && _fontOverride && _scrollX && _scrollY
           && _srt && _fps);

    _fcConfig = FcInitLoadConfigAndFonts();

    _genFonts(_fontName, _fontOverride, false, _fcConfig, gHostIsNatron, kParamFontNameDefault, kParamFontNameAltDefault);

    // Setup selected font
    std::string font, fontName;
    _font->getValue(font);
    int fontID;
    int fontCount = _fontName->getNOptions();
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
            if (!fontFound.empty()) {
                if (fontFound == font) {
                    _fontName->setValue(x);
                    break;
                }
            }
        }
    }
}

TextFXPlugin::~TextFXPlugin()
{
}

void TextFXPlugin::resetCenter(double time) {
    if (!_dstClip) {
        return;
    }
    OfxRectD rod = _dstClip->getRegionOfDefinition(time);
    if ( (rod.x1 <= kOfxFlagInfiniteMin) || (kOfxFlagInfiniteMax <= rod.x2) ||
         ( rod.y1 <= kOfxFlagInfiniteMin) || ( kOfxFlagInfiniteMax <= rod.y2) ) {
        return;
    }
    OfxPointD newCenter;
    newCenter.x = (rod.x1 + rod.x2) / 2;
    newCenter.y = (rod.y1 + rod.y2) / 2;
    if (_position) {
        _position->setValue(newCenter.x, newCenter.y);
    }
}

std::string TextFXPlugin::textFromFile(std::string filename) {
    std::string result;
    if (!filename.empty()) {
        std::ifstream f;
        f.open(filename.c_str());
        std::ostringstream s;
        s << f.rdbuf();
        f.close();
        if (!s.str().empty()) {
            result = s.str();
        }
    }
    return result;
}

// try to parse subtitle
void TextFXPlugin::loadSRT()
{
    std::string filename;
    double fps = 0.0;
    _srt->getValue(filename);
    _fps->getValue(fps);
    if (!RichText::fileExists(filename)) { return; }
    std::vector<RichText::RichTextSubtitle> subtitles = RichText::parseSRT(filename);
    if (subtitles.size()==0) { return; }

    // remove existing keys
    _text->deleteAllKeys();

    for (int i=0;i<subtitles.size();++i) { // add each subtitle at given time
        int startFrame = subtitles.at(i).start*fps;
        int endFrame = subtitles.at(i).end*fps;
        std::string textFrame = subtitles.at(i).str;
        _text->setValueAtTime(startFrame, textFrame); // start frame
        _text->setValueAtTime(endFrame, ""); // end frame
    }
}

void TextFXPlugin::setFontDesc(int stretch, int weight, PangoFontDescription* desc)
{
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
    if (!_dstClip) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(_dstClip);

    // get dstclip
    OFX::auto_ptr<OFX::Image> dstImg(_dstClip->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // renderscale
    checkBadRenderScaleOrField(dstImg, args);

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
    double x, y, r, g, b, a, s_r, s_g, s_b, s_a, strokeWidth, strokeDashX, strokeDashY, strokeDashZ, circleRadius, arcRadius, arcAngle, rotate, scaleX, scaleY, skewX, skewY, scrollX, scrollY, bg_r, bg_g, bg_b, bg_a;
    int fontSize, cwidth, cheight, wrap, align, valign, style, stretch, weight, strokeDash, fontAA, subpixel, hintStyle, hintMetrics, circleWords, letterSpace;
    std::string text, font, txt, fontOverride;
    bool justify = false;
    bool markup = false;
    bool autoSize = false;
    bool move = false;
    bool scaleUniform = false;
    bool centerInteract = false;

    _text->getValueAtTime(args.time, text);
    _fontSize->getValueAtTime(args.time, fontSize);
    _textColor->getValueAtTime(args.time, r, g, b, a);
    _bgColor->getValueAtTime(args.time, bg_r, bg_g, bg_b, bg_a);
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
    _justify->getValueAtTime(args.time, justify);
    _wrap->getValueAtTime(args.time, wrap);
    _align->getValueAtTime(args.time, align);
    _valign->getValueAtTime(args.time, valign);
    _markup->getValueAtTime(args.time, markup);
    _style->getValueAtTime(args.time, style);
    auto_->getValueAtTime(args.time, autoSize);
    stretch_->getValueAtTime(args.time, stretch);
    weight_->getValueAtTime(args.time, weight);
    strokeColor_->getValueAtTime(args.time, s_r, s_g, s_b, s_a);
    strokeWidth_->getValueAtTime(args.time, strokeWidth);
    strokeDash_->getValueAtTime(args.time, strokeDash);
    strokeDashPattern_->getValueAtTime(args.time, strokeDashX, strokeDashY, strokeDashZ);
    fontAA_->getValueAtTime(args.time, fontAA);
    subpixel_->getValueAtTime(args.time, subpixel);
    _hintStyle->getValueAtTime(args.time, hintStyle);
    _hintMetrics->getValueAtTime(args.time, hintMetrics);
    _circleRadius->getValueAtTime(args.time, circleRadius);
    _circleWords->getValueAtTime(args.time, circleWords);
    _letterSpace->getValueAtTime(args.time, letterSpace);
    _canvas->getValueAtTime(args.time, cwidth, cheight);
    _arcRadius->getValueAtTime(args.time, arcRadius);
    _arcAngle->getValueAtTime(args.time, arcAngle);
    _rotate->getValueAtTime(args.time, rotate);
    _scale->getValueAtTime(args.time, scaleX, scaleY);
    _position->getValueAtTime(args.time, x, y);
    _move->getValueAtTime(args.time, move);
    _txt->getValueAtTime(args.time, txt);
    _skewX->getValueAtTime(args.time, skewX);
    _skewY->getValueAtTime(args.time, skewY);
    _scaleUniform->getValueAtTime(args.time, scaleUniform);
    _centerInteract->getValueAtTime(args.time, centerInteract);
    _fontOverride->getValueAtTime(args.time, fontOverride);
    _scrollX->getValueAtTime(args.time, scrollX);
    _scrollY->getValueAtTime(args.time, scrollY);

    double ytext = y*args.renderScale.y;
    double xtext = x*args.renderScale.x;
    int tmp_y = dstRod.y2 - dstBounds.y2;
    int tmp_height = dstBounds.y2 - dstBounds.y1;
    ytext = tmp_y + ((tmp_y+tmp_height-1) - ytext);

    if (!txt.empty()) {
        std::string txt_tmp = textFromFile(txt);
        if (!txt_tmp.empty()) {
            text = txt_tmp;
        }
    }

    // no fonts?
    if ( font.empty() ) {
        font = kParamFontNameDefault;
    }

    std::ostringstream pangoFont;
    pangoFont << font;
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
    PangoFontMap* fontmap;

    fontmap = pango_cairo_font_map_get_default();
    if (pango_cairo_font_map_get_font_type((PangoCairoFontMap*)(fontmap)) != CAIRO_FONT_TYPE_FT) {
        fontmap = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
    }
    pango_fc_font_map_set_config((PangoFcFontMap*)fontmap, _fcConfig);
    pango_cairo_font_map_set_default((PangoCairoFontMap*)(fontmap));

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create(surface);

    layout = pango_cairo_create_layout(cr);
    alist = pango_attr_list_new();

    // flip
    cairo_scale(cr, 1.0f, -1.0f);
    cairo_translate(cr, 0.0f, -height);

    cairo_font_options_t* options = cairo_font_options_create();

    cairo_rectangle(cr, 0, 0, width, height);
    cairo_set_source_rgba(cr, bg_r, bg_g, bg_b, bg_a);
    cairo_fill(cr);

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

    if (markup) {
        if ( pango_parse_markup(text.c_str(), -1, 0, NULL, NULL, NULL, NULL) ) {
            pango_layout_set_markup(layout, text.c_str(), -1);
        } else {
            markup = false; // fallback to plain text
        }
    }
    if (!markup) {
        pango_layout_set_text(layout, text.c_str(), -1);
    }

    desc = pango_font_description_from_string(pangoFont.str().c_str());
    setFontDesc(stretch, weight, desc);

    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    if (!autoSize && !move) {
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

    if (!move) {
        switch(align) {
        case 0:
            pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
            break;
        case 1:
            pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
            break;
        case 2:
            pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
            break;
        }

        if (valign != 0) {
            int text_width, text_height;
            pango_layout_get_pixel_size(layout, &text_width, &text_height);
            switch (valign) {
            case 1:
                cairo_move_to(cr, 0, (height-text_height)/2);
                break;
            case 2:
                cairo_move_to(cr, 0, height-text_height);
                break;
            }
        }

        if (justify) {
            pango_layout_set_justify(layout, true);
        }
    }

    if (letterSpace != 0) {
        pango_attr_list_insert(alist,pango_attr_letter_spacing_new(std::floor((letterSpace*PANGO_SCALE) * args.renderScale.x + 0.5)));
    }

    if (!markup) {
        pango_layout_set_attributes(layout,alist);
    }

    if (!move && !autoSize && (scrollX!=0||scrollY!=0) && arcAngle==0 && circleRadius==0) {
        cairo_move_to(cr, std::floor(scrollX * args.renderScale.x), std::floor(scrollY * args.renderScale.x));
    }

    if (!autoSize && !markup && circleRadius==0 && arcAngle==0 && strokeWidth==0 && move) {
        int moveX, moveY;
        if (centerInteract) {
            int text_width, text_height;
            pango_layout_get_pixel_size(layout, &text_width, &text_height);
            moveX=xtext-(text_width/2);
            moveY=ytext-(text_height/2);
        } else {
            moveX=xtext;
            moveY=ytext;
        }
        cairo_move_to(cr, moveX, moveY);
    }

    if (scaleX!=1.0||scaleY!=1.0) {
        if (!autoSize) {
            cairo_translate(cr, xtext, ytext);
            if (scaleUniform) {
                cairo_scale(cr, scaleX, scaleX);
            } else {
                cairo_scale(cr, scaleX, scaleY);
            }
            cairo_translate(cr, -xtext, -ytext);
        }
    }

    if (skewX != 0.0 && !autoSize) {
        cairo_matrix_t matrixSkewX = {
            1.0, 0.0,
            -skewX, 1.0,
            0.0, 0.0
        };
        cairo_translate(cr, xtext, ytext);
        cairo_transform(cr, &matrixSkewX);
        cairo_translate(cr, -xtext, -ytext);
    }

    if (skewY !=0.0 && !autoSize) {
        cairo_matrix_t matrixSkewY = {
            1.0, -skewY,
            0.0, 1.0,
            0.0, 0.0
        };
        cairo_translate(cr, xtext, ytext);
        cairo_transform(cr, &matrixSkewY);
        cairo_translate(cr, -xtext, -ytext);
    }

    if (rotate !=0 && !autoSize) {
        double rotateX = width/2.0;
        double rotateY = height/2.0;
        if (move) {
            rotateX = xtext;
            rotateY = ytext;
        }
        cairo_translate(cr, rotateX, rotateY);
        cairo_rotate(cr, -rotate * (M_PI/180.0));
        cairo_translate(cr, -rotateX, -rotateY);
    }

    if (strokeWidth>0 && arcAngle==0) {
        if (circleRadius==0) {
            if (strokeDash>0) {
                if (strokeDashX<0.1) {
                    strokeDashX=0.1;
                }
                if (strokeDashY<0) {
                    strokeDashY=0;
                }
                if (strokeDashZ<0) {
                    strokeDashZ=0;
                }
                double dash[] = {strokeDashX, strokeDashY, strokeDashZ};
                cairo_set_dash(cr, dash, strokeDash, 0);
            }

            cairo_new_path(cr);

            if (!move && !autoSize && (scrollX!=0||scrollY!=0)) {
                cairo_move_to(cr, std::floor(scrollX * args.renderScale.x), std::floor(scrollY * args.renderScale.x));
            }

            if (autoSize) {
                cairo_move_to(cr, std::floor((strokeWidth/2) * args.renderScale.x + 0.5), 0.0);
            } else if (move) {
                int moveX, moveY;
                if (centerInteract) {
                    int text_width, text_height;
                    pango_layout_get_pixel_size(layout, &text_width, &text_height);
                    moveX=xtext-(text_width/2);
                    moveY=ytext-(text_height/2);
                } else {
                    moveX=xtext;
                    moveY=ytext;
                }
                cairo_move_to(cr, moveX, moveY);
            }

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
            if (arcAngle>0) {
                double arcX = width/2.0;
                double arcY = height/2.0;
                if (move) {
                    arcX = xtext;
                    arcY = ytext;
                }
                cairo_arc(cr, arcX, arcY, std::floor(arcRadius * args.renderScale.x + 0.5), 0.0, arcAngle * (M_PI/180.0));
                cairo_path_t *path;
                cairo_save(cr);
                path = cairo_copy_path_flat(cr);
                cairo_new_path(cr);
                pango_cairo_layout_line_path(cr, pango_layout_get_line_readonly(layout, 0)); //TODO if more than one line add support for that
                map_path_onto(cr, path);
                cairo_path_destroy(path);
                cairo_set_source_rgba(cr, r, g, b, a);
                cairo_fill(cr);
            }
            else {
                if (!autoSize && move) {
                    int moveX, moveY;
                    if (centerInteract) {
                        int text_width, text_height;
                        pango_layout_get_pixel_size(layout, &text_width, &text_height);
                        moveX=xtext-(text_width/2);
                        moveY=ytext-(text_height/2);
                    } else {
                        moveX=xtext;
                        moveY=ytext;
                    }
                    cairo_move_to(cr, moveX, moveY);
                }
                cairo_set_source_rgba(cr, r, g, b, a);
                pango_cairo_update_layout(cr, layout);
                pango_cairo_show_layout(cr, layout);
            }
        }
    }

    if (circleRadius>0 && !autoSize) {
        double circleX = width/2.0;
        double circleY = height/2.0;
        if (move) {
            circleX = xtext;
            circleY = ytext;
        }
        cairo_translate(cr, circleX, circleY);
        for (int i = 0; i < circleWords; i++) {
            int rwidth, rheight;
            double angle = (360. * i) / circleWords;
            cairo_save(cr);
            cairo_set_source_rgba(cr, r, g, b, a);
            cairo_rotate (cr, angle * G_PI / 180.);
            pango_cairo_update_layout(cr, layout);
            pango_layout_get_size(layout, &rwidth, &rheight);
            cairo_move_to(cr, - ((double)rwidth / PANGO_SCALE) / 2, - std::floor(circleRadius * args.renderScale.x + 0.5));
            pango_cairo_layout_path(cr, layout);
            cairo_fill(cr);
            cairo_restore (cr);
        }
    }

    // cairo status?
    status = cairo_status(cr);
    if (status) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // flush
    cairo_surface_flush(surface);

    // get pixels
    unsigned char* cdata = cairo_image_surface_get_data(surface);
    float* pixelData = (float*)dstImg->getPixelData();

    // write output
    int offset = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixelData[offset + 0] = cdata[offset + 2] * (1.f / 255);
            pixelData[offset + 1] = cdata[offset + 1] * (1.f / 255);
            pixelData[offset + 2] = cdata[offset + 0] * (1.f / 255);
            pixelData[offset + 3] = cdata[offset + 3] * (1.f / 255);
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
}

void TextFXPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (paramName == kParamTransformResetCenterOld) {
        resetCenter(args.time);
    } else if (paramName == kParamFontName) {
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
    } else if (paramName == kParamFontOverride && args.reason != OFX::eChangeTime) {
        int fontID ;
        int fontCount = _fontName->getNOptions();
        _fontName->getValueAtTime(args.time, fontID);
        std::string font;
        if (fontID < fontCount) {
            _fontName->getOption(fontID, font);
            // cascade menu
            if (font.length() > 2 && font[1] == '/' && gHostIsNatron) {
                font.erase(0,2);
            }
        }
        // no fonts?
        if ( font.empty() ) {
            font = kParamFontNameDefault;
        }
        _genFonts(_fontName, _fontOverride, false, _fcConfig, gHostIsNatron, font, kParamFontNameAltDefault);
    } else if (paramName == kParamSubtitleFile) {
        loadSRT(); // load subtitle
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
    bool autoSize = false;

    _canvas->getValue(width, height);
    auto_->getValue(autoSize);

    if (autoSize) {
        int fontSize, style, stretch, weight, letterSpace;
        double strokeWidth;
        std::string text, font, txt;
        bool markup = false;

        _text->getValueAtTime(args.time, text);
        _fontSize->getValueAtTime(args.time, fontSize);
        _font->getValueAtTime(args.time, font);
        _style->getValueAtTime(args.time, style);
        _markup->getValueAtTime(args.time, markup);
        stretch_->getValueAtTime(args.time, stretch);
        weight_->getValueAtTime(args.time, weight);
        strokeWidth_->getValueAtTime(args.time, strokeWidth);
        _letterSpace->getValueAtTime(args.time, letterSpace);
        _txt->getValueAtTime(args.time, txt);

        if (!txt.empty()) {
            std::string txt_tmp = textFromFile(txt);
            if (!txt_tmp.empty()) {
                text = txt_tmp;
            }
        }

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

        // no fonts?
        if ( font.empty() ) {
            font = kParamFontNameDefault;
        }
        std::ostringstream pangoFont;
        pangoFont << font;
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
        PangoFontMap* fontmap;

        fontmap = pango_cairo_font_map_get_default();
        if (pango_cairo_font_map_get_font_type((PangoCairoFontMap*)(fontmap)) != CAIRO_FONT_TYPE_FT) {
            fontmap = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT);
        }
        pango_fc_font_map_set_config((PangoFcFontMap*)fontmap, _fcConfig);
        pango_cairo_font_map_set_default((PangoCairoFontMap*)(fontmap));

        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr = cairo_create(surface);

        layout = pango_cairo_create_layout(cr);
        alist = pango_attr_list_new();

        if (markup) {
            if ( pango_parse_markup(text.c_str(), -1, 0, NULL, NULL, NULL, NULL) ) {
                pango_layout_set_markup(layout, text.c_str(), -1);
            } else {
                markup = false; // fallback to plain text
            }
        }
        if (!markup) {
            pango_layout_set_text(layout, text.c_str(), -1);
        }

        desc = pango_font_description_from_string(pangoFont.str().c_str());
        setFontDesc(stretch, weight, desc);

        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);

        if (letterSpace != 0) {
            pango_attr_list_insert(alist,pango_attr_letter_spacing_new(letterSpace*PANGO_SCALE));
        }

        if (!markup)
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

    if (width>0 && height>0) {
        rod.x1 = rod.y1 = 0;
        rod.x2 = width;
        rod.y2 = height;
    }
    else {
        //rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        //rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
        return false;
    }

    return true;
}

bool TextFXPlugin::getTimeDomain(OfxRangeD &range)
{
    // this should only be called in the general context, ever!
    if (getContext() == eContextGeneral) {
        assert(_range);
        // how many frames on the input clip
        //OfxRangeD srcRange = _srcClip->getFrameRange();

        int min, max;
        _range->getValue(min, max);
        range.min = min;
        range.max = max;

        return true;
    }

    return false;
}

mDeclarePluginFactory(TextFXPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void TextFXPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("Advanced text generator node using Pango and Cairo.");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // add other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);

    Transform3x3Describe(desc, true);
    desc.setOverlayInteractDescriptor(new TransformOverlayDescriptorOldParams);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TextFXPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
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
    PageParamDescriptor *page = desc.definePageParam("Controls");
    ofxsTransformDescribeParams(desc, page, NULL, /*isOpen=*/ true, /*oldParams=*/ true, /*hasAmount=*/ true, /*noTranslate=*/ true);
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamPositionMove);
        param->setLabel(kParamPositionMoveLabel);
        param->setHint(kParamPositionMoveHint);
        param->setDefault(kParamPositionMoveDefault);
        param->setAnimates(false);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamAutoSize);
        param->setLabel(kParamAutoSizeLabel);
        param->setHint(kParamAutoSizeHint);
        param->setAnimates(false);
        param->setDefault(kParamAutoSizeDefault);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamCenterInteract);
        param->setLabel(kParamCenterInteractLabel);
        param->setHint(kParamCenterInteractHint);
        param->setDefault(kParamCenterInteractDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamMarkup);
        param->setLabel(kParamMarkupLabel);
        param->setHint(kParamMarkupHint);
        param->setDefault(kParamMarkupDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamTextFile);
        param->setLabel(kParamTextFileLabel);
        param->setHint(kParamTextFileHint);
        param->setStringType(eStringTypeFilePath);
        param->setFilePathExists(true);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamSubtitleFile);
        param->setLabel(kParamSubtitleFileLabel);
        param->setHint(kParamSubtitleFileHint);
        param->setStringType(eStringTypeFilePath);
        param->setFilePathExists(true);
        param->setAnimates(false);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamFPS);
        param->setLabel(kParamFPSLabel);
        param->setHint(kParamFPSHint);
        param->setDefault(kParamFPSDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamText);
        param->setLabel(kParamTextLabel);
        param->setHint(kParamTextHint);
        param->setStringType(eStringTypeMultiLine);
        param->setAnimates(true);
        param->setDefault("Enter text");
        if (page) {
            page->addChild(*param);
        }
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamJustify);
        param->setLabel(kParamJustifyLabel);
        param->setHint(kParamJustifyHint);
        param->setDefault(kParamJustifyDefault);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
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
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamVAlign);
        param->setLabel(kParamVAlignLabel);
        param->setHint(kParamVAlignHint);
        param->appendOption("Top");
        param->appendOption("Center");
        param->appendOption("Bottom");
        param->setDefault(kParamVAlignDefault);
        param->setAnimates(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamFontName);
        param->setLabel(kParamFontNameLabel);
        param->setHint(kParamFontNameHint);
        if (gHostIsNatron) {
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);
        }
        std::list<std::string>::const_iterator font;
        std::list<std::string> fonts = _genFonts(NULL, NULL, false, NULL, false, kParamFontNameDefault, kParamFontNameAltDefault);
        int defaultFont = 0;
        int altFont = 0;
        int fontIndex = 0;
        for(font = fonts.begin(); font != fonts.end(); ++font) {
            std::string fontName = *font;
            if (!fontName.empty()) {
                std::string fontItem;
                if (gHostIsNatron) {
                    fontItem=fontName[0];
                    fontItem.append("/" + fontName);
                } else {
                    fontItem=fontName;
                }
                param->appendOption(fontItem);
                if (fontName == kParamFontNameDefault) {
                    defaultFont=fontIndex;
                }
                if (fontName == kParamFontNameAltDefault) {
                    altFont=fontIndex;
                }
            }
            fontIndex++;
        }
        if (defaultFont > 0) {
            param->setDefault(defaultFont);
        } else if (altFont > 0) {
            param->setDefault(altFont);
        }
        param->setAnimates(false);
        if (fonts.empty()) {
            param->appendOption("N/A");
        }
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamFontOverride);
        param->setLabel(kParamFontOverrideLabel);
        param->setHint(kParamFontOverrideHint);
        param->setStringType(eStringTypeFilePath);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamFont);
        param->setLabel(kParamFontLabel);
        param->setHint(kParamFontHint);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(false);

        #ifdef DEBUG
        param->setIsSecret(false);
        #else
        param->setIsSecret(true);
        #endif

        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamFontSize);
        param->setLabel(kParamFontSizeLabel);
        param->setHint(kParamFontSizeHint);
        param->setRange(1, 10000);
        param->setDisplayRange(1, 500);
        param->setDefault(kParamFontSizeDefault);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamTextColor);
        param->setLabel(kParamTextColorLabel);
        param->setHint(kParamTextColorHint);
        param->setDefault(1., 1., 1., 1.);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamBGColor);
        param->setLabel(kParamBGColorLabel);
        param->setHint(kParamBGColorHint);
        param->setDefault(0., 0., 0., 0.);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamLetterSpace);
        param->setLabel(kParamLetterSpaceLabel);
        param->setHint(kParamLetterSpaceHint);
        param->setRange(-10000, 10000);
        param->setDisplayRange(-250, 250);
        param->setDefault(kParamLetterSpaceDefault);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamStrokeWidth);
        param->setLabel(kParamStrokeWidthLabel);
        param->setHint(kParamStrokeWidthHint);
        param->setRange(0, 500);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamStrokeWidthDefault);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamStrokeColor);
        param->setLabel(kParamStrokeColorLabel);
        param->setHint(kParamStrokeColorHint);
        param->setDefault(1., 0., 0., 1.);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamStrokeDash);
        param->setLabel(kParamStrokeDashLabel);
        param->setHint(kParamStrokeDashHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamStrokeDashDefault);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        Double3DParamDescriptor* param = desc.defineDouble3DParam(kParamStrokeDashPattern);
        param->setLabel(kParamStrokeDashPatternLabel);
        param->setHint(kParamStrokeDashPatternHint);
        param->setDefault(1.0, 0.0, 0.0);
        param->setAnimates(true);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamCircleRadius);
        param->setLabel(kParamCircleRadiusLabel);
        param->setHint(kParamCircleRadiusHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 1000);
        param->setDefault(kParamCircleRadiusDefault);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
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
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamArcRadius);
        param->setLabel(kParamArcRadiusLabel);
        param->setHint(kParamArcRadiusHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 1000);
        param->setDefault(kParamArcRadiusDefault);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamArcAngle);
        param->setLabel(kParamArcAngleLabel);
        param->setHint(kParamArcAngleHint);
        param->setRange(0, 360);
        param->setDisplayRange(0, 360);
        param->setDefault(kParamArcAngleDefault);
        param->setAnimates(true);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamScrollX);
        param->setLabel(kParamScrollXLabel);
        param->setHint(kParamScrollXHint);
        param->setRange(-10000, 10000);
        param->setDisplayRange(-4000, 4000);
        param->setDefault(kParamScrollXDefault);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamScrollY);
        param->setLabel(kParamScrollYLabel);
        param->setHint(kParamScrollYHint);
        param->setRange(-10000, 10000);
        param->setDisplayRange(-4000, 4000);
        param->setDefault(kParamScrollYDefault);
        param->setAnimates(true);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        if (page) {
            page->addChild(*param);
        }
    }
    // range
    if (context == eContextGeneral) {
        Int2DParamDescriptor *param = desc.defineInt2DParam(kParamGeneratorRange);
        param->setLabel(kParamGeneratorRangeLabel);
        param->setHint(kParamGeneratorRangeHint);
        param->setDefault(1, 1);
        param->setDimensionLabels("min", "max");
        param->setAnimates(false); // can not animate, because it defines the time domain
        if (page) {
            page->addChild(*param);
        }
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TextFXPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TextFXPlugin(handle);
}

static TextFXPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
