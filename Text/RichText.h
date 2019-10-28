/*
####################################################################
#
# Copyright (C) 2019 Ole-André Rodlie <ole.andre.rodlie@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
####################################################################
*/

#include <string>
#include <vector>
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#include <fontconfig/fontconfig.h>

class RichText
{
public:
    enum RichTextAlignment
    {
        RichTextAlignLeft,
        RichTextAlignRight,
        RichTextAlignCenter
    };
    enum RichTextWrap
    {
        RichTextWrapWord,
        RichTextWrapChar,
        RichTextWrapWordChar
    };
    enum RichTextHintStyle
    {
        RichTextHintDefault,
        RichTextHintNone,
        RichTextHintSlight,
        RichTextHintMedium,
        RichTextHintFull
    };
    enum RichTextHintMetrics
    {
        RichTextHintMetricsDefault,
        RichTextHintMetricsOn,
        RichTextHintMetricsOff
    };
    enum RichTextFontAntialias
    {
        RichTextFontAntialiasDefault,
        RichTextFontAntialiasNone,
        RichTextFontAntialiasGray,
        RichTextFontAntialiasSubpixel
    };
    enum RichTextFontSubpixel
    {
        RichTextFontSubpixelDefault,
        RichTextFontSubpixelRGB,
        RichTextFontSubpixelBGR,
        RichTextFontSubpixelVRGB,
        RichTextFontSubpixelVBGR
    };
    enum RichTextFontStretch
    {
        RichTextFontStretchUltraCondensed,
        RichTextFontStretchExtraCondensed,
        RichTextFontStretchCondensed,
        RichTextFontStretchSemiCondensed,
        RichTextFontStretchNormal,
        RichTextFontStretchSemiExpanded,
        RichTextFontStretchExpanded,
        RichTextFontStretchExtraExpanded,
        RichTextFontStretchUltraExpanded
    };
    enum RichTextFontWeight
    {
        RichTextFontWeightThin,
        RichTextFontWeightUltraLight,
        RichTextFontWeightLight,
        RichTextFontWeightSemiLight,
        RichTextFontWeightBook,
        RichTextFontWeightNormal,
        RichTextFontWeightMedium,
        RichTextFontWeightSemiBold,
        RichTextFontWeightBold,
        RichTextFontWeightUltraBold,
        RichTextFontWeightHeavy,
        RichTextFontWeightUltraHeavy
    };
    struct RichTextRenderResult
    {
        bool success;
        unsigned char* buffer;
        int sW; // cairo surface width
        int sH; // cairo surface height
        int pW; // pango layout width
        int pH; // pango layout height
    };
    struct RichTextSubtitle
    {
        double start;
        double end;
        std::string str;
    };

    /** @brief file exists? */
    static bool fileExists(const std::string &str);

    /** @brief extract string from start to end */
    static const std::string extract(const std::string &str,
                                     const std::string &start,
                                     const std::string &end);

    /** @brief replace from to in string */
    static bool replace(std::string& str,
                        const std::string& from,
                        const std::string& to);

    /** @brief trim string */
    static const std::string trimmed(const std::string &str,
                                     bool whitespace = true,
                                     bool singlequote = true,
                                     bool doublequote = true);

    /** @brief does string contain what? */
    static bool contains(const std::string &str,
                         const std::string &what);

    /** @brief does string start with what? */
    static bool startsWith(const std::string &str,
                           const std::string &what);

    /** @brief convert string (HH:MM:SS,MS) to double */
    static double strTimeToSecs(const std::string &str);

    /** @brief is string html? */
    static bool isHtml(const std::string &str);

    /** @brief is string "rich text" html? */
    static bool isRichText(const std::string &str,
                           bool strictMode = true);

    /** @brief is string legacy Natron "rich text" html? */
    static bool isNatronLegacyRichText(const std::string &str);

    /** @brief is string valid Pango markup? */
    static bool isMarkup(const std::string &str);

    /** @brief convert html to pango markup */
    static const std::string convertHtmlToMarkup(const std::string &str,
                                                 double renderScale = 0.0);

    /** @brief set pango layout text align */
    static void setLayoutAlign(PangoLayout *layout,
                               int align = RichTextAlignLeft);

    /** @brief set pango layout word wrap */
    static void setLayoutWrap(PangoLayout *layout,
                              int wrap = RichTextWrapWord);

    /** @brief set pango layout text justify */
    static void setLayoutJustify(PangoLayout *layout,
                                 bool justify = true);

    /** @brief set pango layout markup */
    static void setLayoutMarkup(PangoLayout *layout,
                                const std::string &str,
                                double renderScale = 0.0);

    /** @brief set pango layout width */
    static void setLayoutWidth(PangoLayout *layout,
                               int width);

    /** @brief set font hint style */
    static void setFontHintStyleOption(cairo_font_options_t *options,
                                       int hint = RichTextHintDefault);

    /** @brief set font hint metrics */
    static void setFontHintMetricsOption(cairo_font_options_t *options,
                                         int metric = RichTextHintMetricsDefault);

    /** @brief set font antialias */
    static void setFontAntialiasOption(cairo_font_options_t *options,
                                       int antialias = RichTextFontAntialiasDefault);

    /** @brief set font subpixel order */
    static void setFontSubpixelOption(cairo_font_options_t *options,
                                      int subpixel = RichTextFontSubpixelDefault);

    /** @brief set font stretch */
    static void setFontStretchDescription(PangoFontDescription *description,
                                          int stretch = RichTextFontStretchNormal);

    /** @brief set font weight */
    static void setFontWeightDescription(PangoFontDescription *description,
                                         int weight = RichTextFontWeightNormal);

    /** @brief setup pango fontmap (freetype+fontconfig) */
    static void setupFontmap(FcConfig *fc,
                             PangoFontMap *map);

    /** @brief render rich text to buffer */
    static RichTextRenderResult renderRichText(int width,
                                               int height,
                                               FcConfig *fc,
                                               const std::string &html,
                                               int wrap,
                                               int align,
                                               int justify,
                                               double rX,
                                               double rY,
                                               bool flip = false,
                                               bool noBuffer = false);

    /** @brief parse SRT subtitle file */
    static std::vector<RichTextSubtitle> parseSRT(const std::string &filename);
};
