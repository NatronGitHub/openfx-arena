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

#include "RichText.h"
#include "ofxsImageEffect.h"

#include <iostream>
#include <stdlib.h>

#define kPluginName "RichTextOFX"
#define kPluginIdentifier "net.fxarena.openfx.RichText"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 8
#define kPluginGrouping "Draw"
#define kPluginDescription "OpenFX Rich Text Generator for Natron.\n\nUnder development, require changes in Natron to work as intended."

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe

#define kParamHTML "text"
#define kParamHTMLLabel "Text"
#define kParamHTMLHint "The text that will be drawn."

// https://doc.qt.io/qt-5/richtext-html-subset.html
// this is the default html code produced by Qt
#define kParamHTMLDefault \
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">" \
"<html>" \
"<head>" \
"<meta name=\"qrichtext\" content=\"1\" />" \
"<style type=\"text/css\">" \
"p, li { white-space: pre-wrap; }" \
"</style>" \
"</head>" \
"<body style=\" font-family:'Droid Sans'; font-size:21pt; font-weight:400; font-style:normal;\">" \
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">OpenFX Rich Text</p>" \
"</body>" \
"</html>" \

// should be done by host editor
// need to spec something compatible with qrichtext
#define kParamAlign "align"
#define kParamAlignLabel "Align"
#define kParamAlignHint "Text alignment"

// should be done by host editor
// need to spec something compatible with qrichtext
#define kParamWrap "wrap"
#define kParamWrapLabel "Wrap"
#define kParamWrapHint "Word wrap"

// should be done by host editor
// need to spec something compatible with qrichtext
#define kParamJustify "justify"
#define kParamJustifyLabel "Justify"
#define kParamJustifyHint "Text justify."
#define kParamJustifyDefault false

// canvas size
#define kParamCanvas "custom"
#define kParamCanvasLabel "Custom size"
#define kParamCanvasHint "Set custom size, default (0) is project format. Disabled if auto size is active."
#define kParamCanvasDefault 0

// auto size
#define kParamAutoSize "auto"
#define kParamAutoSizeLabel "Auto size"
#define kParamAutoSizeHint "Set sized based on text layout. This will disable custom and project size."
#define kParamAutoSizeDefault false

static std::string ofxPath;

using namespace OFX;

class RichTextPlugin : public ImageEffect
{
public:
    RichTextPlugin(OfxImageEffectHandle handle);
    virtual void render(const RenderArguments &args) override final;
    virtual void changedParam(const InstanceChangedArgs &args,
                              const std::string &paramName) override final;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args,
                                       OfxRectD &rod) override final;

private:
    Clip *_dstClip;
    StringParam *_srcText;
    ChoiceParam *_srcAlign;
    ChoiceParam *_srcWrap;
    BooleanParam *_srcJustify;
    FcConfig *_fc;
    BooleanParam *_srcAuto;
    Int2DParam *_srcCanvas;
};

RichTextPlugin::RichTextPlugin(OfxImageEffectHandle handle)
: ImageEffect(handle)
, _dstClip(nullptr)
, _srcText(nullptr)
, _srcAlign(nullptr)
, _srcWrap(nullptr)
, _srcJustify(nullptr)
, _srcAuto(nullptr)
, _srcCanvas(nullptr)
{
    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    assert(_dstClip && _dstClip->getPixelComponents() == ePixelComponentRGBA);

    _srcText = fetchStringParam(kParamHTML);
    _srcAlign = fetchChoiceParam(kParamAlign);
    _srcWrap = fetchChoiceParam(kParamWrap);
    _srcJustify = fetchBooleanParam(kParamJustify);
    _srcAuto = fetchBooleanParam(kParamAutoSize);
    _srcCanvas = fetchInt2DParam(kParamCanvas);
    assert(_srcText && _srcAlign && _srcWrap &&
           _srcJustify && _srcAuto && _srcCanvas);

    // setup fontconfig
    std::string fontConf = ofxPath;
    fontConf.append("/Contents/Resources/fonts");
    if (RichText::fileExists(fontConf+"/fonts.conf")) {
#ifdef _WIN32
        _putenv_s("FONTCONFIG_PATH", fontConf.c_str());
#else
        setenv("FONTCONFIG_PATH", fontConf.c_str(), 1);
#endif
    }
    _fc = FcInitLoadConfigAndFonts();
}

/* Override the render */
void RichTextPlugin::render(const RenderArguments &args)
{
    // renderscale
    if (!kSupportsRenderScale &&
        (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // dstclip
    if (!_dstClip) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(_dstClip);

    // get dstclip
    auto_ptr<Image> dstImg(_dstClip->fetchImage(args.time));
    if (!dstImg.get()) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // renderscale
    checkBadRenderScaleOrField(dstImg, args);

    // get bitdepth
    BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != eBitDepthFloat) {
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get channels
    PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != ePixelComponentRGBA) {
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    OfxRectI dstRod = dstImg->getRegionOfDefinition();
    if (args.renderWindow.x1 < dstBounds.x1 ||
        args.renderWindow.x1 >= dstBounds.x2 ||
        args.renderWindow.y1 < dstBounds.y1 ||
        args.renderWindow.y1 >= dstBounds.y2 ||
        args.renderWindow.x2 <= dstBounds.x1 ||
        args.renderWindow.x2 > dstBounds.x2 ||
        args.renderWindow.y2 <= dstBounds.y1 ||
        args.renderWindow.y2 > dstBounds.y2)
    {
        throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // get options
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    std::string html;
    int align = RichText::RichTextAlignLeft;
    int wrap = RichText::RichTextWrapWord;
    bool justify = kParamJustifyDefault;

    _srcText->getValueAtTime(args.time, html);
    _srcAlign->getValueAtTime(args.time, align);
    _srcWrap->getValueAtTime(args.time, wrap);
    _srcJustify->getValueAtTime(args.time, justify);

    // render image
    RichText::RichTextRenderResult result = RichText::renderRichText(width,
                                                                     height,
                                                                     _fc,
                                                                     html,
                                                                     wrap,
                                                                     align,
                                                                     justify,
                                                                     args.renderScale.x,
                                                                     args.renderScale.y,
                                                                     true /* flip */);
    if (!result.success || (result.sW != width || result.sH != height)) {
        setPersistentMessage(Message::eMessageError, "", "RichText Renderer failed");
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // write output
    float* pixelData = (float*)dstImg->getPixelData();
    int offset = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixelData[offset + 0] = result.buffer[offset + 0] * (1.f / 255);
            pixelData[offset + 1] = result.buffer[offset + 1] * (1.f / 255);
            pixelData[offset + 2] = result.buffer[offset + 2] * (1.f / 255);
            pixelData[offset + 3] = result.buffer[offset + 3] * (1.f / 255);
            offset += 4;
        }
    }
    delete [] result.buffer;
    result.buffer = nullptr;
    pixelData = nullptr;
}

void RichTextPlugin::changedParam(const InstanceChangedArgs &args,
                                  const std::string &paramName)
{
    clearPersistentMessage();
    if (!kSupportsRenderScale &&
        (args.renderScale.x != 1. || args.renderScale.y != 1.))
    {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }
}

bool RichTextPlugin::getRegionOfDefinition(const RegionOfDefinitionArguments &args,
                                           OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }

    // get size options
    int width = -1;
    int height = -1;
    bool autoSize = false;
    _srcCanvas->getValue(width, height);
    _srcAuto->getValue(autoSize);

    // get layout size?
    if (autoSize) {
        // get render options
        std::string html;
        int align = RichText::RichTextAlignLeft;
        int wrap = RichText::RichTextWrapWord;
        bool justify = kParamJustifyDefault;
        _srcText->getValueAtTime(args.time, html);
        _srcAlign->getValueAtTime(args.time, align);
        _srcWrap->getValueAtTime(args.time, wrap);
        _srcJustify->getValueAtTime(args.time, justify);

        // render layout
        RichText::RichTextRenderResult result = RichText::renderRichText(0,
                                                                         0,
                                                                         _fc,
                                                                         html,
                                                                         wrap,
                                                                         align,
                                                                         justify,
                                                                         0.0,
                                                                         0.0,
                                                                         true /* flip */,
                                                                         true /* no buffer */);
        if (result.success) {
            width = result.pW;
            height = result.pH;
        }
    }
    // set new width/height
    if (width>0 && height>0) {
        rod.x1 = rod.y1 = 0;
        rod.x2 = width;
        rod.y2 = height;
    }
    else { return false; }
    return true;
}

mDeclarePluginFactory(RichTextPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void RichTextPluginFactory::describe(ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

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
void RichTextPluginFactory::describeInContext(ImageEffectDescriptor &desc,
                                              ContextEnum /*context*/)
{
    // set path
    std::string path;
    ofxPath = desc.getPropertySet().propGetString(kOfxPluginPropFilePath, false);

    // there has to be an input clip, even for generators
    ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->setOptional(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam("Controls");
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamHTML);
        param->setLabel(kParamHTMLLabel);
        param->setHint(kParamHTMLHint);
        param->setStringType(eStringTypeRichTextFormat);
        param->setAnimates(false); // Disable for now, don't work on Natron 2.3.15, need modifications
        param->setDefault(kParamHTMLDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamAlign);
        param->setLabel(kParamAlignLabel);
        param->setHint(kParamAlignHint);
        param->appendOption("Left"); // RichTextAlignLeft
        param->appendOption("Right"); // RichTextAlignRight
        param->appendOption("Center"); // RichTextAlignCenter
        param->setDefault(RichText::RichTextAlignLeft);
        param->setAnimates(false);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamWrap);
        param->setLabel(kParamWrapLabel);
        param->setHint(kParamWrapHint);
        param->appendOption("Word"); // RichTextWrapWord
        param->appendOption("Char"); // RichTextWrapChar
        param->appendOption("Word-Char"); // RichTextWrapWordChar
        param->setDefault(RichText::RichTextWrapWord);
        param->setAnimates(false);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamJustify);
        param->setLabel(kParamJustifyLabel);
        param->setHint(kParamJustifyHint);
        param->setDefault(kParamJustifyDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        Int2DParamDescriptor *param = desc.defineInt2DParam(kParamCanvas);
        param->setLabel(kParamCanvasLabel);
        param->setHint(kParamCanvasHint);
        param->setDefault(kParamCanvasDefault, kParamCanvasDefault);
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
        param->setDefault(kParamAutoSizeDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* RichTextPluginFactory::createInstance(OfxImageEffectHandle handle,
                                                   ContextEnum /*context*/)
{
    return new RichTextPlugin(handle);
}

static RichTextPluginFactory p(kPluginIdentifier,
                               kPluginVersionMajor,
                               kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
