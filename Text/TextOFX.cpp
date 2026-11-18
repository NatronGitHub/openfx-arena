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
#include <mutex>

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION // Yes, we are still doing OpenGL 2.1
#endif
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

#include "fx.h"
#include "PackedImage.h"
#include "RichText.h" // common text related functions (will be renamed to CommonText in the future)

#define kPluginName "TextOFX"
#define kPluginGrouping "Draw"
#define kPluginIdentifier "net.fxarena.openfx.Text"
// 7.0: the Center is in canonical coordinates (the pixel aspect ratio and the RoD origin are
// applied), so the text is where the overlay shows it, and the text is drawn in canonical
// units, so that it isn't squeezed or sheared on a PAR-corrected viewer. The Canvas size is in
// pixels. 6.x is kept, unchanged and deprecated, for old projects (issue #70).
#define kPluginVersionMajor 7
#define kPluginVersionMinor 0
// 6.17: the param set of 6.14 (Natron 2.5.0), with the bug fixes made since. The params added in
// 6.15 (Line spacing) and 6.16 (relative size and position) were never released: they are now
// in 7.0 only.
#define kPluginVersionMajorV6 6
#define kPluginVersionMinorV6 17

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
#define kParamFontSizeMin 1.
#define kParamFontSizeMax 10000.

// The "frame" that relative sizes and positions refer to: the Canvas size if it is set and
// Auto size is off, else the format (the output format in Natron, the project size elsewhere).
#define kFrameDescription "The frame is the Canvas size if it is set and Auto size is off, else the format (in Natron, the format of the output, which is the project format unless the Source input is connected)."

#define kParamSizeUnit "sizeUnit"
#define kParamSizeUnitLabel "Size unit"
#define kParamSizeUnitHint "Unit of the font size. Use a size relative to the frame height so that the text scales with the format. Changing the unit converts the font size so that the text keeps its size, unless 'Font size' or 'Font size (%)' is animated. Only the font size and the position can be relative: the stroke size, the letter and line spacing, the radii and the scroll stay in pixels. " kFrameDescription
#define kParamSizeUnitOptionPixels "Pixels", "The font size is given by 'Font size', in pixels.", "pixels"
#define kParamSizeUnitOptionFrameHeight "% of frame height", "The font size is given by 'Font size (%)', as a percentage of the frame height.", "frameheight"
enum SizeUnitEnum
{
    eSizeUnitPixels = 0,
    eSizeUnitFrameHeight
};
#define kParamSizeUnitDefault eSizeUnitPixels

#define kParamFontSizeRelative "sizeRelative"
#define kParamFontSizeRelativeLabel "Font size (%)"
#define kParamFontSizeRelativeHint "Font size as a percentage of the frame height, used if 'Size unit' is '% of frame height'. It is in the same unit as 'Font size': 5% of a 1080-pixel-high frame is the same as a 'Font size' of 54. The stroke size, the letter and line spacing and the radii stay in pixels. " kFrameDescription
#define kParamFontSizeRelativeDefault 5.
#define kParamFontSizeRelativeMin 0.01
#define kParamFontSizeRelativeMax 1000.

#define kParamPositionRelative "positionRelative"
#define kParamPositionRelativeLabel "Relative position"
#define kParamPositionRelativeHint "If checked, the Center is a position relative to the frame, (0,0) being its bottom-left corner and (1,1) its top-right corner, so that the text stays at the same place in the frame when the format changes. Checking or unchecking this converts the Center (and its keyframes) so that the text does not move. Only the position and the font size can be relative: the stroke size, the letter and line spacing, the radii and the scroll stay in pixels. Note that resetting the Center parameter to its default sets it to the center of the format in canonical coordinates, which is off-frame in relative mode: use the 'Reset Center' button instead. In scripts, set 'Relative position' before setting the Center, since changing it converts the Center. " kFrameDescription
#define kParamPositionRelativeDefault false

// Secret params holding the mode in which the stored size and Center are expressed. They,
// not the user-facing 'Size unit' and 'Relative position', decide how render(), the RoD and
// the interact interpret the values: Natron can restore the user-facing params without any
// instanceChanged (e.g. undoing a reset to default), and the conversion is not on its undo stack.
// Natron sends kOfxChangeUserEdited for many changes that are not user edits (restore defaults,
// link/expression refresh, undo), so instanceChanged alone cannot tell whether to convert:
// convert only if the new mode differs from the stored one. Restoring defaults resets these too.
#define kParamSizeUnitState "sizeUnitState"
#define kParamPositionRelativeState "positionRelativeState"

#define kParamFontName "name"
#define kParamFontNameLabel "Select font"
#define kParamFontNameHint "Select font family to be used.\n\nThis parameter is only used to set font family in the 'font' parameter. This parameter does not support animation, use the 'font' parameter for animation."
#define kParamFontNameDefault "Arial"
#define kParamFontNameAltDefault "DejaVu Sans" // failsafe on Linux/BSD

#define kParamFont "font"
#define kParamFontLabel "Font family"
#define kParamFontHint "The name of the font to be used.\n\nThis parameter can also be used to animate the font family."

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

#define kParamLineSpace "lineSpace"
#define kParamLineSpaceLabel "Line spacing"
#define kParamLineSpaceHint "Extra space between lines, in pixels (may be negative)."
#define kParamLineSpaceDefault 0.

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
#define kParamCanvasHintV7 "Set canvas size in pixels, default (0) is project format. Disabled if auto size is active."
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

    // if no config was given, use a temporary one and release it before returning
    FcConfig *tmpConfig = NULL;
    if (!fontConfig) {
        fontConfig = tmpConfig = FcInitLoadConfigAndFonts();
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
        FcPattern *font = fs->fonts[i]; // owned by fs
        FcChar8 *s = font ? FcPatternFormat(font,(const FcChar8 *)"%{family[0]}") : NULL;
        if (s) {
            std::string fontName(reinterpret_cast<char*>(s));
            fonts.push_back(fontName);
            FcStrFree(s);
        }
    }
    if (fs) {
        FcFontSetDestroy(fs); // also destroys the patterns it contains
    }
    if (os) {
        FcObjectSetDestroy(os);
    }
    if (p) {
        FcPatternDestroy(p);
    }
    if (tmpConfig) {
        FcConfigDestroy(tmpConfig);
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

// Make the calling thread's default pango fontmap a fontconfig/FreeType one that uses fcConfig.
// The default fontmap is per-thread, so this does not affect other render threads.
// pango_fc_font_map_set_config() takes its own reference on fcConfig (pango >= 1.42).
static void
setupFontmap(FcConfig *fcConfig)
{
    PangoFontMap* fontmap = pango_cairo_font_map_get_default(); // transfer none
    if (pango_cairo_font_map_get_font_type((PangoCairoFontMap*)(fontmap)) != CAIRO_FONT_TYPE_FT) {
        // e.g. CoreText on macOS, Win32 on Windows
        PangoFontMap* ftFontmap = pango_cairo_font_map_new_for_font_type(CAIRO_FONT_TYPE_FT); // transfer full, may be NULL
        if (!ftFontmap) {
            return; // pango was built without FreeType support: keep the default fontmap
        }
        pango_cairo_font_map_set_default((PangoCairoFontMap*)(ftFontmap)); // takes its own reference
        g_object_unref(ftFontmap);
        fontmap = ftFontmap;
    }
    pango_fc_font_map_set_config((PangoFcFontMap*)fontmap, fcConfig);
}

class TextFXPlugin : public OFX::ImageEffect
{
public:
    TextFXPlugin(OfxImageEffectHandle handle, bool v7);
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

    /** @brief The frame that relative font sizes and positions refer to, in canonical coordinates. */
    OfxRectD getRelativeFrame(double time, bool autoSize) const;
    OfxRectD getRelativeFrame(double time) const;

    bool isPositionRelative(double time) const;
    int getSizeUnit(double time) const;

    /** @brief return a new reference to the current fontconfig config (may be NULL), release it with FcConfigDestroy() */
    FcConfig* fcConfigRef();

private:
    // The font size in the unit of the size param (scaled by renderScale in render).
    double getFontSize(double time, bool autoSize) const;
    void convertSizeUnit(double time, int sizeUnit);
    bool convertPositionRelative(double time, bool relative);
    double getPAR() const;
    void updateParamsState();
    void updatePositionIncrement(bool relative);

    // From v7, the Center is in canonical coordinates in render(), the text is drawn in
    // canonical units (render scale aside), and the Canvas is in pixels. v6 took the Center as
    // pixels at scale 1 from the RoD origin, drew in pixels, and took the Canvas as a canonical
    // size, which is only right if PAR = 1 and the RoD starts at (0,0) (issue #70). v6 renders
    // as before, for old projects.
    // Line spacing, and the relative size and position (and their state params) exist in v7
    // only: in v6 these pointers are NULL.
    const bool _v7;

    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *_dstClip;
    OFX::StringParam *_text;
    OFX::IntParam *_fontSize;
    OFX::ChoiceParam *_sizeUnit;
    OFX::DoubleParam *_fontSizeRelative;
    OFX::BooleanParam *_positionRelative;
    OFX::IntParam *_sizeUnitState;
    OFX::BooleanParam *_positionRelativeState;
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
    OFX::DoubleParam *_lineSpace;
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
    OFX::ChoiceParam *_skewOrder; // used from v7
    OFX::BooleanParam *_centerInteract;
    OFX::ChoiceParam *_fontName;
    OFX::StringParam *_fontOverride;
    // After construction, _fcConfig is replaced (never modified) under _fcConfigMutex,
    // because render threads may be using it concurrently (see fcConfigRef()).
    FcConfig* _fcConfig;
    std::mutex _fcConfigMutex;
    std::list<std::string> _fontOverrideFiles; // font files added to _fcConfig, in order
    OFX::DoubleParam *_scrollX;
    OFX::DoubleParam *_scrollY;
    OFX::StringParam *_srt;
    OFX::DoubleParam *_fps;
    OFX::Int2DParam  *_range;
};

TextFXPlugin::TextFXPlugin(OfxImageEffectHandle handle, bool v7)
: OFX::ImageEffect(handle)
, _v7(v7)
, _dstClip(NULL)
, _text(NULL)
, _fontSize(NULL)
, _sizeUnit(NULL)
, _fontSizeRelative(NULL)
, _positionRelative(NULL)
, _sizeUnitState(NULL)
, _positionRelativeState(NULL)
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
, _lineSpace(NULL)
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
, _skewOrder(NULL)
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
    if (_v7) {
        _sizeUnit = fetchChoiceParam(kParamSizeUnit);
        _fontSizeRelative = fetchDoubleParam(kParamFontSizeRelative);
        _positionRelative = fetchBooleanParam(kParamPositionRelative);
        _sizeUnitState = fetchIntParam(kParamSizeUnitState);
        _positionRelativeState = fetchBooleanParam(kParamPositionRelativeState);
        _lineSpace = fetchDoubleParam(kParamLineSpace);
    }
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
    _skewOrder = fetchChoiceParam(kParamTransformSkewOrderOld);
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

    assert(!_v7 || (_sizeUnit && _fontSizeRelative && _positionRelative && _sizeUnitState && _positionRelativeState && _lineSpace));
    assert(_text && _fontSize && _fontName && _textColor && _bgColor && _font && _wrap
           && _justify && _align && _valign && _markup && _style && auto_ && stretch_ && weight_ && strokeColor_
           && strokeWidth_ && strokeDash_ && strokeDashPattern_ && fontAA_ && subpixel_ && _hintStyle
           && _hintMetrics && _circleRadius && _circleWords && _letterSpace && _canvas
           && _arcRadius && _arcAngle && _rotate && _scale && _position && _move && _txt
           && _skewX && _skewY && _scaleUniform && _skewOrder && _centerInteract && _fontOverride && _scrollX && _scrollY
           && _srt && _fps);

    _fcConfig = FcInitLoadConfigAndFonts();

    _genFonts(_fontName, _fontOverride, false, _fcConfig, gHostIsNatron, kParamFontNameDefault, kParamFontNameAltDefault);
    {
        std::string fontCustom;
        _fontOverride->getValue(fontCustom);
        if (!fontCustom.empty()) {
            _fontOverrideFiles.push_back(fontCustom);
        }
    }

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

    // need to set a default or else font will be empty if we reset to default
    _font->setDefault(font.empty()? fontName : font);

    if (_v7) {
        updateParamsState();
        if ( _positionRelativeState->getValue() ) {
            updatePositionIncrement(true);
        }
    }
}

TextFXPlugin::~TextFXPlugin()
{
    // the render threads' fontmaps hold their own reference, if any
    if (_fcConfig) {
        FcConfigDestroy(_fcConfig);
    }
}

FcConfig*
TextFXPlugin::fcConfigRef()
{
    std::lock_guard<std::mutex> lock(_fcConfigMutex);
    if (_fcConfig) {
        FcConfigReference(_fcConfig);
    }
    return _fcConfig;
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
    if ( isPositionRelative(time) ) {
        const OfxRectD frame = getRelativeFrame(time);
        if ( (frame.x2 <= frame.x1) || (frame.y2 <= frame.y1) ) {
            return;
        }
        newCenter.x = (newCenter.x - frame.x1) / (frame.x2 - frame.x1);
        newCenter.y = (newCenter.y - frame.y1) / (frame.y2 - frame.y1);
    }
    if (_position) {
        _position->setValue(newCenter.x, newCenter.y);
    }
}

// The canonical size of a width in pixels at scale 1. Hosts get the pixel RoD as
// ceil(x2 * renderScale / PAR) (e.g. Natron's RectD::toPixelEnclosing), and w * PAR / PAR can
// be w plus a rounding error (e.g. w = 31 and PAR = 16/15), which would add a column: make it
// slightly smaller, by far more than any rounding error, even with -ffast-math.
static double
pixelWidthToCanonical(int w, double par)
{
    if (par == 1.) {
        return w;
    }

    return w * par * (1. - 1e-12);
}

OfxRectD TextFXPlugin::getRelativeFrame(double time, bool autoSize) const
{
    OfxRectD frame = {0., 0., 0., 0.};
    if (!autoSize) {
        int cwidth = 0, cheight = 0;
        _canvas->getValueAtTime(time, cwidth, cheight);
        // same condition as in getRegionOfDefinition(), where the canvas becomes the RoD
        if (cwidth > 0 && cheight > 0) {
            // v7: the canvas is in pixels (v6: canonical)
            frame.x2 = _v7 ? pixelWidthToCanonical(cwidth, getPAR()) : cwidth;
            frame.y2 = cheight;
            return frame;
        }
    }
#ifdef OFX_EXTENSIONS_NATRON
    // kOfxImageClipPropFormat (Natron) is the format of the stream, in pixel coordinates.
    // It is the project format if Source is not connected. Other hosts leave it empty.
    OfxRectI format;
    _dstClip->getFormat(format);
    if (format.x2 > format.x1 && format.y2 > format.y1) {
        const double par = _dstClip->getPixelAspectRatio();
        frame.x1 = format.x1 * par;
        frame.x2 = format.x2 * par;
        frame.y1 = format.y1;
        frame.y2 = format.y2;
        return frame;
    }
#endif
    // kOfxImageEffectPropProjectSize/Offset are in canonical coordinates
    const OfxPointD size = getProjectSize();
    const OfxPointD offset = getProjectOffset();
    frame.x1 = offset.x;
    frame.y1 = offset.y;
    frame.x2 = offset.x + size.x;
    frame.y2 = offset.y + size.y;
    return frame;
}

OfxRectD TextFXPlugin::getRelativeFrame(double time) const
{
    return getRelativeFrame(time, auto_->getValueAtTime(time));
}

bool TextFXPlugin::isPositionRelative(double time) const
{
    if (!_positionRelativeState) {
        return false; // v6
    }
    return _positionRelativeState->getValueAtTime(time);
}

int TextFXPlugin::getSizeUnit(double time) const
{
    if (!_sizeUnitState) {
        return eSizeUnitPixels; // v6
    }
    return _sizeUnitState->getValueAtTime(time);
}

double TextFXPlugin::getFontSize(double time, bool autoSize) const
{
    if (getSizeUnit(time) == eSizeUnitFrameHeight) {
        const OfxRectD frame = getRelativeFrame(time, autoSize);
        return (frame.y2 - frame.y1) * _fontSizeRelative->getValueAtTime(time) / 100.;
    }
    return _fontSize->getValueAtTime(time);
}

// An edit block that does nothing if the host does not support them
// (paramEditBegin/paramEditEnd are optional in the parameter suite).
class OptionalEditBlock
{
public:
    OptionalEditBlock(OFX::ParamSet& paramSet, const std::string& name)
        : _paramSet(paramSet)
        , _open(false)
    {
        try {
            _paramSet.beginEditBlock(name);
            _open = true;
        } catch (const std::exception&) {
        }
    }

    ~OptionalEditBlock()
    {
        if (_open) {
            try {
                _paramSet.endEditBlock();
            } catch (const std::exception&) {
            }
        }
    }

private:
    OFX::ParamSet& _paramSet;
    bool _open;
};

// Number of keyframes, 0 if the host cannot tell (the keyframe functions of the parameter
// suite are optional).
static int
getNumKeys(OFX::ValueParam* param)
{
    try {
        return param->getNumKeys();
    } catch (const OFX::Exception::Suite&) {
        return 0;
    }
}

// Convert the font size to the new unit, so that the text keeps its size.
// Animated sizes are left as they are.
void TextFXPlugin::convertSizeUnit(double time, int sizeUnit)
{
    if (getNumKeys(_fontSize) > 0 || getNumKeys(_fontSizeRelative) > 0) {
        return;
    }
    const OfxRectD frame = getRelativeFrame(time);
    const double height = frame.y2 - frame.y1;
    if (height <= 0.) {
        return;
    }
    if (sizeUnit == eSizeUnitFrameHeight) {
        // clamp to the param range
        const double fontSizeRelative = _fontSize->getValue() * 100. / height;
        _fontSizeRelative->setValue( (std::max)(kParamFontSizeRelativeMin, (std::min)(fontSizeRelative, kParamFontSizeRelativeMax)) );
    } else {
        const double fontSize = std::floor(_fontSizeRelative->getValue() * height / 100. + 0.5);
        _fontSize->setValue( (int)(std::max)(kParamFontSizeMin, (std::min)(fontSize, kParamFontSizeMax)) );
    }
}

static OfxPointD
convertPosition(const OfxPointD& p, const OfxRectD& frame, bool toRelative)
{
    OfxPointD ret;
    if (toRelative) {
        ret.x = (p.x - frame.x1) / (frame.x2 - frame.x1);
        ret.y = (p.y - frame.y1) / (frame.y2 - frame.y1);
    } else {
        ret.x = frame.x1 + p.x * (frame.x2 - frame.x1);
        ret.y = frame.y1 + p.y * (frame.y2 - frame.y1);
    }
    return ret;
}

double TextFXPlugin::getPAR() const
{
    const double par = _dstClip->getPixelAspectRatio();
    return (par > 0.) ? par : 1.;
}

// Convert the Center (and its keyframes) to/from relative coordinates, so that the rendered
// text does not move (v7 only: the Center and the frame are canonical).
bool TextFXPlugin::convertPositionRelative(double time, bool relative)
{
    const OfxRectD frame = getRelativeFrame(time);
    if ( (frame.x2 <= frame.x1) || (frame.y2 <= frame.y1) ) {
        return false;
    }
    const int nKeys = getNumKeys(_position);
    const int n = (nKeys == 0) ? 1 : nKeys;
    for (int i = 0; i < n; ++i) {
        const double t = (nKeys == 0) ? time : _position->getKeyTime(i);
        OfxPointD p;
        _position->getValueAtTime(t, p.x, p.y);
        p = convertPosition(p, frame, relative);
        if (nKeys == 0) {
            _position->setValue(p);
        } else {
            _position->setValueAtTime(t, p);
        }
    }

    return true;
}

// A relative Center needs finer steps than the canonical one (see ofxsTransformDescribeParams()).
// kOfxParamPropIncrement and kOfxParamPropDigits are read/write on param instances.
void TextFXPlugin::updatePositionIncrement(bool relative)
{
    try {
        _position->setIncrement(relative ? 0.001 : 1.);
        _position->setDigits(relative ? 4 : 2);
    } catch (const std::exception&) {
        // the host does not let us change them: not a problem
    }
}

void TextFXPlugin::updateParamsState()
{
    const bool pixels = (_sizeUnitState->getValue() == eSizeUnitPixels);
    _fontSize->setEnabled(pixels);
    _fontSizeRelative->setEnabled(!pixels);
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
    double lineSpace = 0.;
    if (_lineSpace) { // v7
        _lineSpace->getValueAtTime(args.time, lineSpace);
    }
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

    double xtext, ytext; // the Center in pixels from the RoD origin, at the render scale
    if (_v7) {
        // v7: the Center is in canonical coordinates, like the overlay. Map it to pixels as in
        // ofxCoordSystem.rst, "X' = (X * SX)/PAR, Y' = Y * SY * FS" (no fields here: FS = 1),
        // from the origin of the RoD, which is in pixel coordinates.
        OfxPointD c = {x, y};
        if ( isPositionRelative(args.time) ) {
            c = convertPosition( c, getRelativeFrame(args.time, autoSize), false );
        }
        const double par = getPAR();
        xtext = c.x / par * args.renderScale.x - dstRod.x1;
        ytext = c.y * args.renderScale.y - dstRod.y1;
    } else {
        // v6, unchanged: the code below takes the position in pixels at scale 1, from the
        // bottom-left corner of the RoD, without the PAR (issue #70).
        ytext = y*args.renderScale.y;
        xtext = x*args.renderScale.x;
    }
    // The cairo surface covers dstRod, whatever the bounds of dstImg (which may be only the
    // render window, see the write below), so flip y within dstRod, not within dstBounds.
    // When dstBounds == dstRod this is the same value as before.
    // In the flipped context below, y = height is the bottom edge of the RoD: v7 maps the Center
    // there exactly, v6 put it one pixel too high.
    ytext = (dstRod.y2 - dstRod.y1 - (_v7 ? 0 : 1)) - ytext;

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
    double fontSizePx = fontSize;
    if (getSizeUnit(args.time) == eSizeUnitPixels) {
        // unchanged, for compatibility
        pangoFont << " " << std::floor(fontSizePx * args.renderScale.x + 0.5);
    } else {
        fontSizePx = getFontSize(args.time, autoSize);
        // Pango treats a size of 0 as unset
        pangoFont << " " << (std::max)(1., std::floor(fontSizePx * args.renderScale.x + 0.5));
    }

    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    // v7 draws in canonical units horizontally (times the render scale): the context below is
    // scaled by 1/PAR in x, so that the glyphs, the rotation, skew and scale, the circle, the
    // arc and the stroke look right on a canonical (PAR-corrected) viewer, as the overlay
    // does. v6 drew everything in pixels (xScale = 1).
    const double xScale = _v7 ? getPAR() : 1.;
    const double drawWidth = width * xScale; // the width of the RoD in drawing units
    xtext *= xScale;

    cairo_t *cr;
    cairo_status_t status;
    cairo_surface_t *surface;
    PangoLayout *layout;
    PangoFontDescription *desc;
    PangoAttrList *alist;

    {
        FcConfig* fcConfig = fcConfigRef();
        setupFontmap(fcConfig);
        if (fcConfig) {
            FcConfigDestroy(fcConfig); // the fontmap holds its own reference
        }
    }

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create(surface);

    layout = pango_cairo_create_layout(cr);
    alist = pango_attr_list_new();
    if (xScale != 1.) {
        // After the layout is created: its context keeps the identity transform, so that it is
        // measured as in getRegionOfDefinition(), and v7 never updates it to the transforms
        // below. With hinted metrics, Pango rounds the advances in device space under a
        // non-uniform transform, which spaced the glyphs out (twice as much at PAR 2), or
        // made them overlap with a non-uniform scale or a skew (v6 does that at any PAR).
        cairo_scale(cr, 1. / xScale, 1.);
    }

    // flip
    cairo_scale(cr, 1.0f, -1.0f);
    cairo_translate(cr, 0.0f, -height);

    cairo_font_options_t* options = cairo_font_options_create();

    cairo_rectangle(cr, 0, 0, drawWidth, height);
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
            pango_layout_set_width(layout, (int)(drawWidth * PANGO_SCALE));
            pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
            break;
        case 2:
            pango_layout_set_width(layout, (int)(drawWidth * PANGO_SCALE));
            pango_layout_set_wrap(layout, PANGO_WRAP_CHAR);
            break;
        case 3:
            pango_layout_set_width(layout, (int)(drawWidth * PANGO_SCALE));
            pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
            break;
        default:
            pango_layout_set_width(layout, -1);
            break;
        }
    }

    // pango_layout_set_spacing() adds space between lines, in Pango units.
    // (pango_layout_set_line_spacing() takes a line-height factor instead.)
    // Set it before the layout is measured for the vertical alignment below.
    if (lineSpace != 0.) {
        pango_layout_set_spacing(layout, (int)std::floor((lineSpace*PANGO_SCALE) * args.renderScale.y + 0.5));
    }

    if (_v7) {
        // v7: the same transform as the overlay draws (TransformInteractHelper and
        // ofxsMatTransformCanonical() in SupportExt), about the Center whether or not Transform
        // is checked, since that is where the handle is ("Rotation angle in degrees around the
        // Center", "Center of rotation and scale"): in canonical coordinates (y up),
        // p' = C + R(rotate) * Skew(skewX, skewY, skewOrder) * Scale(scale) * (p - C),
        // with R counterclockwise. v6 applied rotate first, then skew, then scale, ignored the
        // skew order, and rotated about the center of the RoD if Transform was unchecked.
        // This is done before the alignment, the scroll and the moves below, so that they are
        // transformed too: the whole layout is transformed about the Center (v6 did these
        // before the transform, which moved its pivot by the vertical alignment).
        if ( !autoSize && ( (scaleX != 1.) || (scaleY != 1.) || (skewX != 0.) || (skewY != 0.) || (rotate != 0.) ) ) {
            const double sx = scaleX;
            const double sy = scaleUniform ? scaleX : scaleY;
            const bool skewOrderYX = (_skewOrder->getValueAtTime(args.time) != 0);
            // skew matrix, as ofxsMatSkewXY()
            const double k11 = skewOrderYX ? 1. : (1. + skewX * skewY);
            const double k12 = skewX;
            const double k21 = skewY;
            const double k22 = skewOrderYX ? (1. + skewX * skewY) : 1.;
            const double rads = rotate * (OFXS_PI / 180.0);
            const double c = std::cos(rads);
            const double sn = std::sin(rads);
            // A = R * K * S, in y-up coordinates
            const double a11 = (c * k11 - sn * k21) * sx;
            const double a12 = (c * k12 - sn * k22) * sy;
            const double a21 = (sn * k11 + c * k21) * sx;
            const double a22 = (sn * k12 + c * k22) * sy;
            // The drawing units are canonical (times the render scale) with y down: conjugate
            // A by the flip diag(1, -1). cairo_matrix_t is {xx, yx, xy, yy, x0, y0}.
            cairo_matrix_t m = {
                a11, -a21,
                -a12, a22,
                0., 0.
            };
            cairo_translate(cr, xtext, ytext);
            cairo_transform(cr, &m);
            cairo_translate(cr, -xtext, -ytext);
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
                cairo_translate(cr, 0, (height-text_height)/2);
                break;
            case 2:
                cairo_translate(cr, 0, height-text_height);
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

    // v6 scaled the vertical scroll by the horizontal render scale
    const double scrollYScale = _v7 ? args.renderScale.y : args.renderScale.x;
    if (!move && !autoSize && (scrollX!=0||scrollY!=0) && arcAngle==0 && circleRadius==0) {
        cairo_move_to(cr, std::floor(scrollX * args.renderScale.x), std::floor(scrollY * scrollYScale));
    }

    // Move to where the layout is drawn from: the Center, or the Center minus half the layout
    // size if centerInteract. v6 truncated that point to integers.
    auto moveToLayoutOrigin = [&]() {
        double moveX = xtext;
        double moveY = ytext;
        if (centerInteract) {
            int text_width, text_height;
            pango_layout_get_pixel_size(layout, &text_width, &text_height);
            if (_v7) {
                moveX -= text_width / 2.;
                moveY -= text_height / 2.;
            } else {
                moveX -= text_width / 2;
                moveY -= text_height / 2;
            }
        }
        if (!_v7) {
            moveX = (int)moveX;
            moveY = (int)moveY;
        }
        cairo_move_to(cr, moveX, moveY);
    };

    if (!autoSize && !markup && circleRadius==0 && arcAngle==0 && strokeWidth==0 && move) {
        moveToLayoutOrigin();
    }

    if (!_v7) {
        // v6, unchanged
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
            double rotateX = drawWidth/2.0;
            double rotateY = height/2.0;
            if (move) {
                rotateX = xtext;
                rotateY = ytext;
            }
            cairo_translate(cr, rotateX, rotateY);
            cairo_rotate(cr, -rotate * (OFXS_PI / 180.0));
            cairo_translate(cr, -rotateX, -rotateY);
        }

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
                // only 3 dash lengths exist; strokeDash ranges up to 100,
                // and larger values made cairo read past the end of dash[]
                cairo_set_dash(cr, dash, std::min(strokeDash, 3), 0);
            }

            cairo_new_path(cr);

            if (!move && !autoSize && (scrollX!=0||scrollY!=0)) {
                cairo_move_to(cr, std::floor(scrollX * args.renderScale.x), std::floor(scrollY * scrollYScale));
            }

            if (autoSize) {
                cairo_move_to(cr, std::floor((strokeWidth/2) * args.renderScale.x + 0.5), 0.0);
            } else if (move) {
                moveToLayoutOrigin();
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
                double arcX = drawWidth/2.0;
                double arcY = height/2.0;
                if (move) {
                    arcX = xtext;
                    arcY = ytext;
                }
                cairo_arc(cr, arcX, arcY, std::floor(arcRadius * args.renderScale.x + 0.5), 0.0, arcAngle * (OFXS_PI / 180.0));
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
                    moveToLayoutOrigin();
                }
                cairo_set_source_rgba(cr, r, g, b, a);
                if (!_v7) {
                    // v7 keeps the layout measured as in getRegionOfDefinition() (see the
                    // cairo_scale() above)
                    pango_cairo_update_layout(cr, layout);
                }
                pango_cairo_show_layout(cr, layout);
            }
        }
    }

    if (circleRadius>0 && !autoSize) {
        double circleX = drawWidth/2.0;
        double circleY = height/2.0;
        if (move) {
            circleX = xtext;
            circleY = ytext;
        }
        cairo_translate(cr, circleX, circleY);
        if (_v7) {
            // v7 builds the path of the word once, under an axis-aligned CTM, and rotates the
            // path, not the CTM (issues #86, #88). FreeType 2.15.0 (a98c7c2, fixed in 05f343a
            // after 2.15.0) computes |det| wrongly in FT_Matrix_Check() when the determinant is
            // negative, as with the y flip of this context, so FT_Set_Transform() drops some
            // rotations (0 <= cos 2a <= 1/16, e.g. [43.21, 45.01) and [135, 136.8) degrees) and
            // the glyph outlines come out unrotated, at the rotated glyph positions: e.g. the
            // diagonal words with 8 or 16 words at PAR 1, 1.09 or 1.2. The path is in user
            // coordinates, which don't depend on the CTM: only the hinting does, and it is done
            // once, as for unrotated text, so it is the same for every word.
            // v7 keeps the layout measured as in getRegionOfDefinition() (see the cairo_scale()
            // above), so it is the same for all words.
            int rwidth, rheight;
            pango_layout_get_size(layout, &rwidth, &rheight);
            cairo_matrix_t ctm;
            cairo_get_matrix(cr, &ctm);
            // the CTM of untransformed text: the 1/PAR scale and the flip above, without the
            // transform, at the same origin
            const cairo_matrix_t axisAligned = { 1. / xScale, 0., 0., -1., ctm.x0, ctm.y0 };
            cairo_set_matrix(cr, &axisAligned);
            cairo_new_path(cr);
            cairo_move_to(cr, - ((double)rwidth / PANGO_SCALE) / 2, - std::floor(circleRadius * args.renderScale.x + 0.5));
            pango_cairo_layout_path(cr, layout);
            cairo_path_t* wordPath = cairo_copy_path(cr);
            cairo_new_path(cr);
            cairo_set_matrix(cr, &ctm);
            cairo_set_source_rgba(cr, r, g, b, a);
            for (int i = 0; i < circleWords; i++) {
                const double angle = (360. * i) / circleWords;
                cairo_save(cr);
                cairo_rotate(cr, angle * G_PI / 180.);
                cairo_append_path(cr, wordPath);
                cairo_fill(cr);
                cairo_restore(cr);
            }
            cairo_path_destroy(wordPath);
        } else {
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
    const int cstride = cairo_image_surface_get_stride(surface);

    // write output
    // The surface covers dstRod, its row j being y = dstRod.y1 + j (the context is flipped above).
    // Write the render window, addressing dstImg through its bounds and rowBytes, which may
    // differ from dstRod, include padding, or be negative (kOfxImagePropRowBytes).
    for (int y = args.renderWindow.y1; y < args.renderWindow.y2; y++) {
        float* dst = PackedImage::pixelAddress(dstImg->getPixelData(), dstBounds, dstImg->getRowBytes(), 4, args.renderWindow.x1, y);
        const int sy = y - dstRod.y1;
        for (int x = args.renderWindow.x1; x < args.renderWindow.x2; x++, dst += 4) {
            const int sx = x - dstRod.x1;
            if (sx < 0 || sx >= width || sy < 0 || sy >= height) {
                dst[0] = dst[1] = dst[2] = dst[3] = 0.f;
                continue;
            }
            const unsigned char* src = cdata + (size_t)sy * cstride + (size_t)sx * 4;
            dst[0] = src[2] * (1.f / 255);
            dst[1] = src[1] * (1.f / 255);
            dst[2] = src[0] * (1.f / 255);
            dst[3] = src[3] * (1.f / 255);
        }
    }

    pango_attr_list_unref(alist);
    cairo_font_options_destroy(options);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cdata = NULL;
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
        // Render threads may be using _fcConfig, so do not modify it: build a new config
        // with the same font files plus the new one, and swap it in.
        FcConfig* newConfig = FcInitLoadConfigAndFonts();
        if (newConfig) {
            for (std::list<std::string>::const_iterator it = _fontOverrideFiles.begin(); it != _fontOverrideFiles.end(); ++it) {
                FcConfigAppFontAddFile(newConfig, (const FcChar8 *)it->c_str());
            }
            _genFonts(_fontName, _fontOverride, false, newConfig, gHostIsNatron, font, kParamFontNameAltDefault);
            std::string fontCustom;
            _fontOverride->getValue(fontCustom);
            if (!fontCustom.empty()) {
                _fontOverrideFiles.push_back(fontCustom);
            }
            FcConfig* oldConfig;
            {
                std::lock_guard<std::mutex> lock(_fcConfigMutex);
                oldConfig = _fcConfig;
                _fcConfig = newConfig;
            }
            if (oldConfig) {
                FcConfigDestroy(oldConfig);
            }
        }
    } else if (paramName == kParamSubtitleFile) {
        loadSRT(); // load subtitle
    } else if (!_v7) {
        // v6 has none of the params below
    } else if (paramName == kParamSizeUnit) {
        const int sizeUnit = _sizeUnit->getValueAtTime(args.time);
        if ( sizeUnit != _sizeUnitState->getValue() ) {
            OptionalEditBlock eb(*this, "Convert font size");
            convertSizeUnit(args.time, sizeUnit);
            // The size and the relative size are separate params, so the new unit applies
            // even if the size could not be converted (e.g. animated).
            _sizeUnitState->setValue(sizeUnit);
        }
        updateParamsState();
    } else if (paramName == kParamPositionRelative) {
        const bool relative = _positionRelative->getValueAtTime(args.time);
        if ( relative != _positionRelativeState->getValue() ) {
            OptionalEditBlock eb(*this, "Convert position");
            // Keep the state consistent with the Center: change it only if the Center was
            // converted (it is not if the frame is empty).
            if ( convertPositionRelative(args.time, relative) ) {
                _positionRelativeState->setValue(relative);
            }
        }
        updatePositionIncrement( _positionRelativeState->getValue() );
    } else if ( (paramName == kParamSizeUnitState) || (paramName == kParamPositionRelativeState) ) {
        // e.g. a preset load
        updateParamsState();
        updatePositionIncrement( _positionRelativeState->getValue() );
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

    _canvas->getValueAtTime(args.time, width, height);
    auto_->getValueAtTime(args.time, autoSize);

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
        double lineSpace = 0.;
        if (_lineSpace) { // v7
            _lineSpace->getValueAtTime(args.time, lineSpace);
        }
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
        if (getSizeUnit(args.time) == eSizeUnitPixels) {
            pangoFont << " " << fontSize;
        } else {
            // rounded as in render(), at renderScale 1
            // Pango treats a size of 0 as unset
            pangoFont << " " << (std::max)(1., std::floor(getFontSize(args.time, true) + 0.5));
        }

        cairo_t *cr;
        cairo_surface_t *surface;
        PangoLayout *layout;
        PangoFontDescription *desc;
        PangoAttrList *alist;

        {
            FcConfig* fcConfig = fcConfigRef();
            setupFontmap(fcConfig);
            if (fcConfig) {
                FcConfigDestroy(fcConfig); // the fontmap holds its own reference
            }
        }

        // This surface is only used to measure the layout, and its size does not affect
        // the measurement. (It used to be sized from the output rod, which is uninitialized here.)
        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
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

        if (lineSpace != 0.) {
            pango_layout_set_spacing(layout, (int)std::floor(lineSpace*PANGO_SCALE + 0.5));
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
        // The RoD is in canonical coordinates. v7: the canvas is in pixels, so apply the PAR
        // (v6 did not), and the text is measured in canonical units, as render() draws it.
        rod.x2 = (_v7 && !autoSize) ? pixelWidthToCanonical(width, getPAR()) : width;
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

// The Transform interact, made to work when "Relative position" is checked.
//
// In that case the Center param holds a position relative to the frame, but
// TransformInteractHelper works in canonical coordinates. The helper only uses the
// Center as the origin of its handles (Text has no Translate param), so we make it work by
// shifting everything it draws, and every pen position it gets, by
// shift = canonicalCenter - Center, and by converting back the Center it computes when it
// is dragged. During a drag the shift is frozen, because the helper then works on its own
// copy of the params (_tpDrag), in its shifted space.
class TextTransformInteractHelper
    : public TransformInteractHelperOldParams
{
public:
    TextTransformInteractHelper(OFX::ImageEffect* effect,
                                OFX::Interact* interact)
        : TransformInteractHelperOldParams(effect, interact)
        , _plugin( dynamic_cast<TextFXPlugin*>(effect) )
        , _center(NULL)
        , _scale(NULL)
        , _rotate(NULL)
        , _skewX(NULL)
        , _skewY(NULL)
        , _relativeDrag(false)
    {
        assert(_plugin);
        _center = effect->fetchDouble2DParam(kParamTransformCenterOld);
        _scale = effect->fetchDouble2DParam(kParamTransformScaleOld);
        _rotate = effect->fetchDoubleParam(kParamTransformRotateOld);
        _skewX = effect->fetchDoubleParam(kParamTransformSkewXOld);
        _skewY = effect->fetchDoubleParam(kParamTransformSkewYOld);
        assert(_center && _scale && _rotate && _skewX && _skewY);
        _dragFrame.x1 = _dragFrame.y1 = _dragFrame.x2 = _dragFrame.y2 = 0.;
        _dragShift.x = _dragShift.y = 0.;
        _dragCenter.x = _dragCenter.y = 0.;
    }

    virtual bool draw(const OFX::DrawArgs &args) OVERRIDE
    {
        OfxRectD frame;
        OfxPointD shift;
        if (_relativeDrag) {
            shift = _dragShift;
        } else if ( (_mouseState != eReleased) || !getRelativeShift(args.time, &frame, &shift) ) {
            return TransformInteractHelperOldParams::draw(args);
        }
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslated(shift.x, shift.y, 0.);
        bool ret = TransformInteractHelperOldParams::draw(args);
        glPopMatrix();

        return ret;
    }

    virtual bool penDown(const OFX::PenArgs &args) OVERRIDE
    {
        OfxRectD frame;
        OfxPointD shift;
        if (_relativeDrag) {
            shift = _dragShift;
            frame = _dragFrame;
        } else if ( (_mouseState != eReleased) || !getRelativeShift(args.time, &frame, &shift) ) {
            return TransformInteractHelperOldParams::penDown(args);
        }
        OFX::PenArgs shiftedArgs = shiftArgs(args, shift);
        bool ret = TransformInteractHelperOldParams::penDown(shiftedArgs);
        if ( (_mouseState != eReleased) && !_relativeDrag ) {
            _relativeDrag = true;
            _dragFrame = frame;
            _dragShift = shift;
            _center->getValueAtTime(args.time, _dragCenter.x, _dragCenter.y);
        }

        return ret;
    }

    virtual bool penMotion(const OFX::PenArgs &args) OVERRIDE
    {
        OfxRectD frame;
        OfxPointD shift;
        if (_relativeDrag) {
            shift = _dragShift;
        } else if ( (_mouseState != eReleased) || !getRelativeShift(args.time, &frame, &shift) ) {
            return TransformInteractHelperOldParams::penMotion(args);
        }
        OFX::PenArgs shiftedArgs = shiftArgs(args, shift);
        const bool centerDrag = _relativeDrag && (_mouseState == eDraggingCenter);
        const bool interactiveDrag = _interactiveDrag;
        if (centerDrag) {
            // the helper would set the Center in its own space: set it ourselves below
            _interactiveDrag = false;
        }
        bool ret = TransformInteractHelperOldParams::penMotion(shiftedArgs);
        _interactiveDrag = interactiveDrag;
        if (centerDrag && interactiveDrag && ret) {
            const OfxPointD center = toRelative(_tpDrag.center, args.pixelScale);
            OfxPointD current;
            _center->getValueAtTime(args.time, current.x, current.y);
            if ( (current.x != center.x) || (current.y != center.y) ) {
                _center->setValue(center.x, center.y);
            }
        }

        return ret;
    }

    virtual bool penUp(const OFX::PenArgs &args) OVERRIDE
    {
        if (!_relativeDrag) {
            return TransformInteractHelperOldParams::penUp(args);
        }
        OFX::PenArgs shiftedArgs = shiftArgs(args, _dragShift);
        bool ret;
        if ( (_mouseState != eReleased) && !_interactiveDrag ) {
            // Set the params as the helper would, but with the Center converted.
            const TransformParams& tp = _tpDrag;
            OfxPointD scale;
            ofxsTransformGetScale(tp.scale, tp.scaleUniform, &scale);
            const OfxPointD center = toRelative(tp.center, args.pixelScale);
            {
                ImageEffect::EditBlock eb(*_effect, "Set Transform");
                _center->setValue(center.x, center.y);
                _scale->setValue(scale.x, scale.y);
                _rotate->setValue(tp.rotate);
                _skewX->setValue(tp.skewX);
                _skewY->setValue(tp.skewY);
            }
            _interactiveDrag = true; // so that the helper does not set the params again
            ret = TransformInteractHelperOldParams::penUp(shiftedArgs);
            _interactiveDrag = false;
        } else {
            ret = TransformInteractHelperOldParams::penUp(shiftedArgs);
        }
        _relativeDrag = false;

        return ret;
    }

    virtual void loseFocus(const OFX::FocusArgs &args) OVERRIDE
    {
        // the helper releases the mouse
        _relativeDrag = false;
        TransformInteractHelperOldParams::loseFocus(args);
    }

private:
    // If the position is relative, get the frame and the shift from the helper's space to
    // canonical coordinates, and return true.
    bool getRelativeShift(double time, OfxRectD* frame, OfxPointD* shift) const
    {
        if ( !_plugin || !_plugin->isPositionRelative(time) ) {
            return false;
        }
        *frame = _plugin->getRelativeFrame(time);
        if ( (frame->x2 <= frame->x1) || (frame->y2 <= frame->y1) ) {
            return false;
        }
        OfxPointD center;
        _center->getValueAtTime(time, center.x, center.y);
        const OfxPointD canonical = convertPosition(center, *frame, false);
        shift->x = canonical.x - center.x;
        shift->y = canonical.y - center.y;

        return true;
    }

    static OFX::PenArgs shiftArgs(const OFX::PenArgs &args, const OfxPointD& shift)
    {
        OFX::PenArgs shifted = args;
        shifted.penPosition.x -= shift.x;
        shifted.penPosition.y -= shift.y;

        return shifted;
    }

    // round to the power of 10 below the size of a screen pixel, as the helper does
    static double fround(double val, double pscale)
    {
        const double pscale10 = std::pow( 10., std::floor( std::log10(pscale) ) );

        return pscale10 * std::floor(val / pscale10 + 0.5);
    }

    // Convert a Center in the helper's space during a drag to a relative position.
    OfxPointD toRelative(const OfxPointD& center, const OfxPointD& pixelScale) const
    {
        if ( (center.x == _dragCenter.x) && (center.y == _dragCenter.y) ) {
            // unchanged: avoid rounding errors
            return _dragCenter;
        }
        OfxPointD canonical;
        canonical.x = center.x + _dragShift.x;
        canonical.y = center.y + _dragShift.y;
        OfxPointD ret = convertPosition(canonical, _dragFrame, true);
        ret.x = fround( ret.x, pixelScale.x / (_dragFrame.x2 - _dragFrame.x1) );
        ret.y = fround( ret.y, pixelScale.y / (_dragFrame.y2 - _dragFrame.y1) );

        return ret;
    }

    TextFXPlugin* _plugin;
    OFX::Double2DParam* _center;
    OFX::Double2DParam* _scale;
    OFX::DoubleParam* _rotate;
    OFX::DoubleParam* _skewX;
    OFX::DoubleParam* _skewY;
    bool _relativeDrag; // a drag started in relative mode is in progress
    OfxRectD _dragFrame;
    OfxPointD _dragShift;
    OfxPointD _dragCenter; // the Center when the drag started (in the helper's space, it is unchanged)
};

typedef OverlayInteractFromHelper<TextTransformInteractHelper> TextTransformInteract;

class TextTransformOverlayDescriptor
    : public DefaultEffectOverlayDescriptor<TextTransformOverlayDescriptor, TextTransformInteract>
{
};

mDeclarePluginFactoryVersioned(TextFXPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
template<unsigned int majorVersion>
void
TextFXPluginFactory<majorVersion>::describe(OFX::ImageEffectDescriptor &desc)
{
    if (majorVersion < kPluginVersionMajor) {
        // hidden from the hosts' menus (e.g. Natron), still loaded by old projects
        desc.setIsDeprecated(true);
    }
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
    if (majorVersion >= 7) {
        desc.setOverlayInteractDescriptor(new TextTransformOverlayDescriptor);
    } else {
        // as 6.14: no relative position
        desc.setOverlayInteractDescriptor(new TransformOverlayDescriptorOldParams);
    }
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
template<unsigned int majorVersion>
void
TextFXPluginFactory<majorVersion>::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
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
        if (majorVersion >= 7) {
            param->setLayoutHint(eLayoutHintNoNewLine, 1); // next to Relative position
        }
        if (page) {
            page->addChild(*param);
        }
    }
    if (majorVersion >= 7) { // added in 6.15/6.16, unreleased, now 7.0 only
        {
            BooleanParamDescriptor *param = desc.defineBooleanParam(kParamPositionRelative);
            param->setLabel(kParamPositionRelativeLabel);
            param->setHint(kParamPositionRelativeHint);
            param->setDefault(kParamPositionRelativeDefault);
            param->setAnimates(false);
            if (page) {
                page->addChild(*param);
            }
        }
    }
    if (majorVersion >= 7) { // added in 6.15/6.16, unreleased, now 7.0 only
        {
            BooleanParamDescriptor *param = desc.defineBooleanParam(kParamPositionRelativeState);
            param->setDefault(kParamPositionRelativeDefault);
            param->setIsSecretAndDisabled(true);
            param->setAnimates(false);
            param->setEvaluateOnChange(false);
            if (page) {
                page->addChild(*param);
            }
        }
    }
    if (majorVersion >= 7) { // added in 6.15/6.16, unreleased, now 7.0 only
        {
            IntParamDescriptor *param = desc.defineIntParam(kParamSizeUnitState);
            param->setDefault(kParamSizeUnitDefault);
            param->setIsSecretAndDisabled(true);
            param->setAnimates(false);
            param->setEvaluateOnChange(false);
            if (page) {
                page->addChild(*param);
            }
        }
    }
    {
        Int2DParamDescriptor* param = desc.defineInt2DParam(kParamCanvas);
        param->setLabel(kParamCanvasLabel);
        param->setHint(majorVersion >= 7 ? kParamCanvasHintV7 : kParamCanvasHint);
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
        param->setAnimates(false); // can not animate, the param is dynamic
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
        param->setAnimates(true);

        /*#ifdef DEBUG
        param->setIsSecret(false);
        #else
        param->setIsSecret(true);
        #endif*/

        if (page) {
            page->addChild(*param);
        }
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamFontSize);
        param->setLabel(kParamFontSizeLabel);
        param->setHint(kParamFontSizeHint);
        param->setRange( (int)kParamFontSizeMin, (int)kParamFontSizeMax );
        param->setDisplayRange(1, 500);
        param->setDefault(kParamFontSizeDefault);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    if (majorVersion >= 7) { // added in 6.15/6.16, unreleased, now 7.0 only
        {
            ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamSizeUnit);
            param->setLabel(kParamSizeUnitLabel);
            param->setHint(kParamSizeUnitHint);
            assert(param->getNOptions() == eSizeUnitPixels);
            param->appendOption(kParamSizeUnitOptionPixels);
            assert(param->getNOptions() == eSizeUnitFrameHeight);
            param->appendOption(kParamSizeUnitOptionFrameHeight);
            param->setDefault(kParamSizeUnitDefault);
            param->setAnimates(false);
            param->setLayoutHint(eLayoutHintNoNewLine, 1);
            if (page) {
                page->addChild(*param);
            }
        }
    }
    if (majorVersion >= 7) { // added in 6.15/6.16, unreleased, now 7.0 only
        {
            DoubleParamDescriptor* param = desc.defineDoubleParam(kParamFontSizeRelative);
            param->setLabel(kParamFontSizeRelativeLabel);
            param->setHint(kParamFontSizeRelativeHint);
            param->setRange(kParamFontSizeRelativeMin, kParamFontSizeRelativeMax);
            param->setDisplayRange(0., 50.);
            param->setIncrement(0.1);
            param->setDefault(kParamFontSizeRelativeDefault);
            param->setAnimates(true);
            if (page) {
                page->addChild(*param);
            }
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
        if (majorVersion < 7) {
            param->setLayoutHint(OFX::eLayoutHintDivider); // as 6.14 (v7: after Line spacing)
        }
        if (page) {
            page->addChild(*param);
        }
    }
    if (majorVersion >= 7) { // added in 6.15/6.16, unreleased, now 7.0 only
        {
            DoubleParamDescriptor* param = desc.defineDoubleParam(kParamLineSpace);
            param->setLabel(kParamLineSpaceLabel);
            param->setHint(kParamLineSpaceHint);
            param->setRange(-10000, 10000);
            param->setDisplayRange(-100, 100);
            param->setDefault(kParamLineSpaceDefault);
            param->setLayoutHint(OFX::eLayoutHintDivider);
            if (page) {
                page->addChild(*param);
            }
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
template<unsigned int majorVersion>
ImageEffect*
TextFXPluginFactory<majorVersion>::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TextFXPlugin(handle, /*v7=*/ majorVersion >= 7);
}

// Each version is its own factory class: two versions sharing one class would share its uid.
static TextFXPluginFactory<kPluginVersionMajor> p(kPluginIdentifier, kPluginVersionMinor);
static TextFXPluginFactory<kPluginVersionMajorV6> p6(kPluginIdentifier, kPluginVersionMinorV6);
mRegisterPluginFactoryInstance(p)
mRegisterPluginFactoryInstance(p6)
