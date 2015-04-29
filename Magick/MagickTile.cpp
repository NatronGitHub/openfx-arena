/*

openfx-arena - https://github.com/olear/openfx-arena

Copyright (c) 2015, Ole-André Rodlie <olear@fxarena.net>
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

*  Neither the name of the {organization} nor the names of its
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

*/

#include "MagickTile.h"
#include <iostream>
#include "ofxsMacros.h"
#include <Magick++.h>
#include <sstream>

#define kPluginName "Tile"
#define kPluginGrouping "Transform"
#define kPluginDescription  "Tile image."

#define kPluginIdentifier "net.fxarena.openfx.MagickTile"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

#define kParamRows "rows"
#define kParamRowsLabel "Rows"
#define kParamRowsHint "Rows in grid"
#define kParamRowsDefault 2

#define kParamCols "cols"
#define kParamColsLabel "Colums"
#define kParamColsHint "Columns in grid"
#define kParamColsDefault 2

using namespace OFX;

class MagickTilePlugin : public OFX::ImageEffect
{
public:
    MagickTilePlugin(OfxImageEffectHandle handle);
    virtual ~MagickTilePlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;

private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;

    // params
    OFX::IntParam *rows_;
    OFX::IntParam *cols_;
};

MagickTilePlugin::MagickTilePlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick("");

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    rows_ = fetchIntParam(kParamRows);
    cols_ = fetchIntParam(kParamCols);
    assert(rows_&&cols_);
}

MagickTilePlugin::~MagickTilePlugin()
{
}

/* Override the render */
void MagickTilePlugin::render(const OFX::RenderArguments &args)
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
    int channels = 0;
    if (dstComponents != OFX::ePixelComponentRGB)
        channels = 4;
    else
        channels = 3;

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Get params
    int rows,cols;
    rows_->getValueAtTime(args.time, rows);
    cols_->getValueAtTime(args.time, cols);

    // Setup image
    int magickWidth = args.renderWindow.x2 - args.renderWindow.x1;
    int magickHeight = args.renderWindow.y2 - args.renderWindow.y1;
    int magickWidthStep = magickWidth*channels;
    int magickSize = magickWidth*magickHeight*channels;
    float* magickBlock;
    magickBlock = new float[magickSize];
    std::string colorType;
    if (channels==4)
        colorType="RGBA";
    else
        colorType = "RGB";

    // Read image
    Magick::Image magickImage(magickWidth,magickHeight,colorType,Magick::FloatPixel,(float*)srcImg->getPixelData());

    // Calc thumbs
    int tileWidth = magickWidth/rows;
    int tileHeight = magickHeight/cols;
    int thumbs = rows*cols;

    std::string thumb;
    std::ostringstream makeThumb;
    makeThumb << tileWidth << "x" << tileHeight << "-0-0";
    thumb = makeThumb.str();

    std::string grid;
    std::ostringstream makeGrid;
    makeGrid << rows << "x" << cols;
    grid = makeGrid.str();

    //try {
        // Montage
        Magick::Color backgroundColor("rgba(0,0,0,0)");
        Magick::Montage montageSettings;
        montageSettings.shadow(false);
        montageSettings.backgroundColor(backgroundColor);
        montageSettings.geometry(thumb);
        montageSettings.tile(grid);

        std::list<Magick::Image> montagelist;
        std::list<Magick::Image> imageList;
        for(int y = 0; y < thumbs; y++)
            imageList.push_back(magickImage);

        Magick::montageImages(&montagelist,imageList.begin(),imageList.end(),montageSettings);
        Magick::appendImages(&magickImage,montagelist.begin(),montagelist.end());

        // Write to buffer
        magickImage.write(0,0,magickWidth,magickHeight,colorType,Magick::FloatPixel,magickBlock);

        // Return image
        if (channels==4) { // RGBA
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
        }
        else { // RGB
            for(int y = args.renderWindow.y1; y < (args.renderWindow.y1 + magickHeight); y++) {
                OfxRGBColourF *dstPix = (OfxRGBColourF *)dstImg->getPixelAddress(args.renderWindow.x1, y);
                float *srcPix = (float*)(magickBlock + y * magickWidthStep + args.renderWindow.x1);
                for(int x = args.renderWindow.x1; x < (args.renderWindow.x1 + magickWidth); x++) {
                    dstPix->r = srcPix[0];
                    dstPix->g = srcPix[1];
                    dstPix->b = srcPix[2];
                    dstPix++;
                    srcPix+=3;
                }
            }
        }
    free(magickBlock);
}

mDeclarePluginFactory(MagickTilePluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void MagickTilePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
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
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void MagickTilePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{   
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

    // make some pages
    PageParamDescriptor *page = desc.definePageParam("Tile");
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamRows);
        param->setLabel(kParamRowsLabel);
        param->setHint(kParamRowsHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 100);
        param->setDefault(kParamRowsDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamCols);
        param->setLabel(kParamColsLabel);
        param->setHint(kParamColsHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 100);
        param->setDefault(kParamColsDefault);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* MagickTilePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new MagickTilePlugin(handle);
}

void getMagickTilePluginID(OFX::PluginFactoryArray &ids)
{
    static MagickTilePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
