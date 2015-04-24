/*
 MagickText
 Write text on image using Magick.

 Written by Ole-André Rodlie <olear@fxarena.net>

 Based on https://github.com/MrKepzie/openfx-io/blob/master/OIIO/OIIOText.cpp

 OIIOText plugin
 Write text on images using OIIO.

 Written by Alexandre Gauthier <https://github.com/MrKepzie>

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

#include "MagickText.h"

#include "ofxsProcessing.H"
#include "ofxsCopier.h"
#include "ofxsPositionInteract.h"

#include "ofxNatron.h"
#include "ofxsMacros.h"
#include <fontconfig/fontconfig.h>
#include <Magick++.h>
#include <sstream>
#include <iostream>

#define CLAMP(value, min, max) (((value) >(max)) ? (max) : (((value) <(min)) ? (min) : (value)))

#define kPluginName "Text"
#define kPluginGrouping "Draw"
#define kPluginDescription  "Write text on images."

#define kPluginIdentifier "net.fxarena.openfx.MagickText"
#define kPluginVersionMajor 1 // Incrementing this number means that you have broken backwards compatibility of the plug-in.
#define kPluginVersionMinor 0 // Increment this when you have fixed a bug or made it faster.

#define kSupportsTiles 0 // ???

#define kSupportsMultiResolution 1 // ???
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

#define kParamPosition "position"
#define kParamPositionLabel "Position"
#define kParamPositionHint "The position where starts the baseline of the first character."

#define kParamInteractive "interactive"
#define kParamInteractiveLabel "Interactive"
#define kParamInteractiveHint "When checked the image will be rendered whenever moving the overlay interact instead of when releasing the mouse button."

#define kParamText "text"
#define kParamTextLabel "Text"
#define kParamTextHint "The text that will be drawn on the image"

#define kParamFontSize "fontSize"
#define kParamFontSizeLabel "Size"
#define kParamFontSizeHint "The height of the characters to render in pixels"

#define kParamFontName "fontName"
#define kParamFontNameLabel "Font"
#define kParamFontNameHint "The name of the font to be used. Defaults to some reasonable system font."

#define kParamFontDecor "fontDecor"
#define kParamFontDecorLabel "Decoration"
#define kParamFontDecorHint "Font decoration."

// #define kParamFillCheck "fillColor"
// #define kParamFillCheckLabel "Fill Color"
// #define kParamFillCheckHint "Enable or disable fill color"
// #define kParamFillCheckDefault true

#define kParamTextColor "textColor"
#define kParamTextColorLabel "Fill Color"
#define kParamTextColorHint "The fill color of the text to render"

#define kParamStrokeCheck "strokeCheck"
#define kParamStrokeCheckLabel "Outline"
#define kParamStrokeCheckHint "Enable or disable outline"
#define kParamStrokeCheckDefault false

#define kParamStrokeColor "strokeColor"
#define kParamStrokeColorLabel "Stroke Color"
#define kParamStrokeColorHint "The stroke color of the text to render"

#define kParamStroke "stroke"
#define kParamStrokeLabel "Stroke Width"
#define kParamStrokeHint "Adjust stroke width for outline"
#define kParamStrokeDefault 1

//#define kClipTex "Texture"

using namespace OFX;

class MagickTextPlugin : public OFX::ImageEffect
{
public:

    MagickTextPlugin(OfxImageEffectHandle handle);

    virtual ~MagickTextPlugin();

    /* Override the render */
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;

    /* override is identity */
    virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime) OVERRIDE FINAL;

    /* override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;

    /* override changed clip */
    //virtual void changedClip(const OFX::InstanceChangedArgs &args, const std::string &clipName) OVERRIDE FINAL;

    // override the rod call
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;

    // override the roi call
    //virtual void getRegionsOfInterest(const OFX::RegionsOfInterestArguments &args, OFX::RegionOfInterestSetter &rois) OVERRIDE FINAL;

private:


private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    //OFX::Clip *texClip_;

    OFX::Double2DParam *position_;
    OFX::StringParam *text_;
    OFX::IntParam *fontSize_;
    OFX::ChoiceParam *fontName_;
    OFX::ChoiceParam *fontDecor_;
    OFX::RGBAParam *textColor_;
    OFX::RGBAParam *strokeColor_;
    OFX::BooleanParam *strokeEnabled_;
    //OFX::BooleanParam *fillEnabled_;
    OFX::DoubleParam *strokeWidth_;
};

MagickTextPlugin::MagickTextPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
/*, texClip_(0)*/
{
    Magick::InitializeMagick("");

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    //texClip_ = fetchClip(kClipTex);
    //assert(texClip_ && (texClip_->getPixelComponents() == OFX::ePixelComponentRGBA || texClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    position_ = fetchDouble2DParam(kParamPosition);
    text_ = fetchStringParam(kParamText);
    fontSize_ = fetchIntParam(kParamFontSize);
    fontName_ = fetchChoiceParam(kParamFontName);
    fontDecor_ = fetchChoiceParam(kParamFontDecor);
    textColor_ = fetchRGBAParam(kParamTextColor);
    strokeColor_ = fetchRGBAParam(kParamStrokeColor);
    strokeEnabled_ = fetchBooleanParam(kParamStrokeCheck);
    //fillEnabled_ = fetchBooleanParam(kParamFillCheck);
    strokeWidth_ = fetchDoubleParam(kParamStroke);
    assert(position_ && text_ && fontSize_ && fontName_ && textColor_ && fontDecor_ && strokeColor_ && strokeEnabled_ /*&& fillEnabled_*/ && strokeWidth_);
}

MagickTextPlugin::~MagickTextPlugin()
{
}

/* Override the render */
void MagickTextPlugin::render(const OFX::RenderArguments &args)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (!srcClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(srcClip_);
    std::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    if (srcImg.get()) {
        if (srcImg->getRenderScale().x != args.renderScale.x ||
            srcImg->getRenderScale().y != args.renderScale.y ||
            srcImg->getField() != args.fieldToRender) {
            setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
            OFX::throwSuiteStatusException(kOfxStatFailed);
            return;
        }
    }

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

    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && dstBitDepth != srcImg->getPixelDepth())) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if ((dstComponents != OFX::ePixelComponentRGBA && dstComponents != OFX::ePixelComponentRGB && dstComponents != OFX::ePixelComponentAlpha) ||
        (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
        //throw std::runtime_error("render window outside of image bounds");
    }

    //std::auto_ptr<OFX::Image> texImg(texClip_->fetchImage(args.time));
    //bool use_tex = false;
    /*if (texImg.get()) {
        use_tex=true;
    }*/

    // Get params
    double x, y;
    position_->getValueAtTime(args.time, x, y);
    std::string text;
    text_->getValueAtTime(args.time, text);
    int fontSize;
    fontSize_->getValueAtTime(args.time, fontSize);
    int fontName;
    fontName_->getValueAtTime(args.time, fontName);
    int fontDecor;
    fontDecor_->getValueAtTime(args.time, fontDecor);
    double r, g, b, a;
    textColor_->getValueAtTime(args.time, r, g, b, a);
    float textColor[4];
    textColor[0] = (float)r;
    textColor[1] = (float)g;
    textColor[2] = (float)b;
    textColor[3] = (float)a;
    double r_s, g_s, b_s, a_s;
    strokeColor_->getValueAtTime(args.time, r_s, g_s, b_s, a_s);
    float strokeColor[4];
    strokeColor[0] = (float)r_s;
    strokeColor[1] = (float)g_s;
    strokeColor[2] = (float)b_s;
    strokeColor[3] = (float)a_s;
    bool use_stroke = false;
    strokeEnabled_->getValueAtTime(args.time, use_stroke);
    //bool use_fill = false;
    //fillEnabled_->getValueAtTime(args.time, use_fill);
    double strokeWidth;
    strokeWidth_->getValueAtTime(args.time, strokeWidth);

    // Get font file from fontconfig
    std::string fontFile;
    FcConfig* config = FcInitLoadConfigAndFonts();
    FcPattern* pat = FcPatternCreate();
    FcObjectSet* os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
    FcFontSet* fs = FcFontList(config, pat, os);
    for (int i=0; fs && i < fs->nfont; ++i) {
        FcPattern* font = fs->fonts[i];
        FcChar8 *style, *family;
        if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch && FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch) {
             std::string font_family(reinterpret_cast<char*>(family));
             std::string font_style(reinterpret_cast<char*>(style));
             std::string font_name;
             font_name.append(font_family);
             font_name.append("-");
             font_name.append(font_style);
             std::replace(font_name.begin(),font_name.end(),' ','-');
             if (fontName==i) {
                fontFile = font_name;
                break;
             }
        }
    }
    if (fs)
        FcFontSetDestroy(fs);

    // Read
    int magickWidth = args.renderWindow.x2 - args.renderWindow.x1;
    int magickHeight = args.renderWindow.y2 - args.renderWindow.y1;
    int magickWidthStep = magickWidth*4;
    int magickSize = magickWidth*magickHeight*4;
    float* magickBlock;
    magickBlock = new float[magickSize];
    Magick::Image magickImage(magickWidth,magickHeight,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());

    // Set font size
    magickImage.fontPointsize(fontSize);

    // Set stroke width
    magickImage.strokeWidth(strokeWidth);

    // Convert colors to int
    int rI = ((uint8_t)(255.0f *CLAMP(r, 0.0, 1.0)));
    int gI = ((uint8_t)(255.0f *CLAMP(g, 0.0, 1.0)));
    int bI = ((uint8_t)(255.0f *CLAMP(b, 0.0, 1.0)));
    int r_sI = ((uint8_t)(255.0f *CLAMP(r_s, 0.0, 1.0)));
    int g_sI = ((uint8_t)(255.0f *CLAMP(g_s, 0.0, 1.0)));
    int b_sI = ((uint8_t)(255.0f *CLAMP(b_s, 0.0, 1.0)));

    std::ostringstream rgba;
    rgba << "rgba(" << rI <<"," << gI << "," << bI << "," << a << ")";
    std::string textRGBA = rgba.str();

    std::ostringstream rgba_s;
    rgba_s << "rgba(" << r_sI <<"," << g_sI << "," << b_sI << "," << a_s << ")";
    std::string strokeRGBA = rgba_s.str();

    // Flip image
    //if (!use_tex)
        magickImage.flip();

    // Position x y
    OfxRectI rod,bounds;
    rod = srcImg->getRegionOfDefinition();
    bounds = srcImg->getBounds();

    int ytext = y*args.renderScale.y;
    int xtext = x*args.renderScale.x;
    int tmp_y = rod.y2 - bounds.y2;
    int tmp_height = bounds.y2 - bounds.y1;
    ytext = tmp_y + ((tmp_y+tmp_height-1) - ytext);

    // Setup draw
    std::list<Magick::Drawable> text_draw_list;
    text_draw_list.push_back(Magick::DrawableFont(fontFile));
    text_draw_list.push_back(Magick::DrawableText(xtext, ytext, text));
    //if (use_fill)
        text_draw_list.push_back(Magick::DrawableFillColor(textRGBA));
    if (use_stroke)
        text_draw_list.push_back(Magick::DrawableStrokeColor(strokeRGBA));


    // Text decoration
    if (fontDecor>0) {
        switch(fontDecor) {
        case 1:
            text_draw_list.push_back(Magick::DrawableTextDecoration(Magick::UnderlineDecoration));
            break;
        case 2:
            text_draw_list.push_back(Magick::DrawableTextDecoration(Magick::OverlineDecoration));
            break;
        case 3:
            text_draw_list.push_back(Magick::DrawableTextDecoration(Magick::LineThroughDecoration));
            break;
        default:
            text_draw_list.push_back(Magick::DrawableTextDecoration(Magick::NoDecoration));
            break;
        }
    }

    // Draw
    //if (!use_tex)
        magickImage.draw(text_draw_list);
    //else {
        /*Magick::Image texImage(magickWidth,magickHeight,"RGBA",Magick::FloatPixel,(float*)texImg->getPixelData());
        Magick::Image drawImage;
        drawImage.flip();
        std::ostringstream drawSizeS;
        drawSizeS << magickWidth << "x" << magickHeight;
        std::string drawSize = drawSizeS.str();
        drawImage.size(drawSize);
        drawImage.magick("RGBA");
        Magick::Color backgroundColor("rgba(0,0,0,0)");
        drawImage.backgroundColor(backgroundColor);
        drawImage.draw(text_draw_list);
        drawImage.flip();
        drawImage.composite(texImage,0,0,Magick::OverCompositeOp);
        magickImage.composite(drawImage,0,0,Magick::OverCompositeOp);*/
    //}

    // Flip image
    //if (!use_tex)
        magickImage.flip();

    // Return
    magickImage.write(0,0,magickWidth,magickHeight,"RGBA",Magick::FloatPixel,magickBlock);
    for(int y = args.renderWindow.y1; y < (args.renderWindow.y1 + magickHeight); y++) {
        OfxRGBAColourF *dstPix = (OfxRGBAColourF *)dstImg->getPixelAddress(args.renderWindow.x1, y);
        float *srcPix = (float*)(magickBlock + y * magickWidthStep + args.renderWindow.x1);
        for(int x = args.renderWindow.x1; x < (args.renderWindow.x1 + magickWidth); x++) {
            dstPix->r = srcPix[0];
            dstPix->g = srcPix[1];
            dstPix->b = srcPix[2];
            dstPix->a = srcPix[3];
            dstPix++;
            srcPix+=4;
        }
    }
    free(magickBlock);
}

bool MagickTextPlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &/*identityTime*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }

    std::string text;
    text_->getValueAtTime(args.time, text);
    if (text.empty()) {
        identityClip = srcClip_;
        return true;
    }

    double r, g, b, a;
    textColor_->getValueAtTime(args.time, r, g, b, a);
    if (a == 0.) {
        identityClip = srcClip_;
        return true;
    }

    double r_s, g_s, b_s, a_s;
    strokeColor_->getValueAtTime(args.time, r_s, g_s, b_s, a_s);
    if (a_s == 0.) {
        identityClip = srcClip_;
        return true;
    }

    return false;
}

void MagickTextPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &/*paramName*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    clearPersistentMessage();
}

bool MagickTextPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(MagickTextPluginFactory, {}, {});

namespace {
struct PositionInteractParam {
    static const char *name() { return kParamPosition; }
    static const char *interactiveName() { return kParamInteractive; }
};
}

/** @brief The basic describe function, passed a plugin descriptor */
void MagickTextPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    /*desc.addSupportedBitDepth(eBitDepthUByte);
    desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthHalf);*/
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles); // may be switched to true later?
    desc.setSupportsMultiResolution(kSupportsMultiResolution); // may be switch to true later? don't forget to reduce font size too
    desc.setRenderThreadSafety(kRenderThreadSafety);

    desc.setOverlayInteractDescriptor(new PositionOverlayDescriptor<PositionInteractParam>);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void MagickTextPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    //gHostIsNatron = (OFX::getImageEffectHostDescription()->hostName == kNatronOfxHostName);
    
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentRGB);
    dstClip->setSupportsTiles(kSupportsTiles);

    /*ClipDescriptor *texClip = desc.defineClip(kClipTex);
    texClip->addSupportedComponent(ePixelComponentRGBA);
    texClip->addSupportedComponent(ePixelComponentRGB);
    texClip->addSupportedComponent(ePixelComponentAlpha);
    texClip->setTemporalClipAccess(false);
    texClip->setSupportsTiles(kSupportsTiles);
    texClip->setOptional(true);*/

    // make some pages and to things in
    PageParamDescriptor *page = desc.definePageParam("Text");

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
        if (hostHasNativeOverlayForPosition) {
            param->setUseHostOverlayHandle(true);
        }
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamInteractive);
        param->setLabel(kParamInteractiveLabel);
        param->setHint(kParamInteractiveHint);
        param->setAnimates(false);
        page->addChild(*param);
        
        //Do not show this parameter if the host handles the interact
        if (hostHasNativeOverlayForPosition) {
            param->setIsSecret(true);
        }
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
        IntParamDescriptor* param = desc.defineIntParam(kParamFontSize);
        param->setLabel(kParamFontSizeLabel);
        param->setHint(kParamFontSizeHint);
        param->setDefault(16);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamFontName);
        param->setLabel(kParamFontNameLabel);
        param->setHint(kParamFontNameHint);

        // Get all fonts from fontconfig
        FcConfig* config = FcInitLoadConfigAndFonts();
        FcPattern* pat = FcPatternCreate();
        FcObjectSet* os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
        FcFontSet* fs = FcFontList(config, pat, os);
        for (int i=0; fs && i < fs->nfont; ++i) {
            FcPattern* font = fs->fonts[i];
            FcChar8 *style, *family;
            if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch && FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch) {
                 std::string font_family(reinterpret_cast<char*>(family));
                 std::string font_style(reinterpret_cast<char*>(style));
                 std::string font_name;
                 font_name.append(font_family);
                 font_name.append("-");
                 font_name.append(font_style);
                 std::replace(font_name.begin(),font_name.end(),' ','-');
                 param->appendOption(font_name);
            }
        }
        if (fs)
            FcFontSetDestroy(fs);

        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamFontDecor);
        param->setLabel(kParamFontDecorLabel);
        param->setHint(kParamFontDecorHint);
        param->appendOption("None");
        param->appendOption("Underline");
        param->appendOption("Overline");
        param->appendOption("Strike-through");
        param->setAnimates(true);
        page->addChild(*param);
    }
    /*{
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamFillCheck);
        param->setLabel(kParamFillCheckLabel);
        param->setHint(kParamFillCheckHint);
        param->setEvaluateOnChange(false);
        param->setDefault(kParamFillCheckDefault);
        page->addChild(*param);
    }*/
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamTextColor);
        param->setLabel(kParamTextColorLabel);
        param->setHint(kParamTextColorHint);
        param->setDefault(1., 1., 1., 1.);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamStrokeCheck);
        param->setLabel(kParamStrokeCheckLabel);
        param->setHint(kParamStrokeCheckHint);
        param->setEvaluateOnChange(false);
        param->setDefault(kParamStrokeCheckDefault);
        page->addChild(*param);
    }
    {
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamStrokeColor);
        param->setLabel(kParamStrokeColorLabel);
        param->setHint(kParamStrokeColorHint);
        param->setDefault(1., 1., 1., 1.);
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamStroke);
        param->setLabel(kParamStrokeLabel);
        param->setHint(kParamStrokeHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamStrokeDefault);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* MagickTextPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new MagickTextPlugin(handle);
}

void getMagickTextPluginID(OFX::PluginFactoryArray &ids)
{
    static MagickTextPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
