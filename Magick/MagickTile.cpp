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

#include "ofxsMacros.h"
#include <Magick++.h>
#include <magick/MagickCore.h>
#include <sstream>

#define kPluginName "Tile"
#define kPluginGrouping "Transform"
#define kPluginDescription  "Tile image."

#define kPluginIdentifier "net.fxarena.openfx.MagickTile"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kParamRows "rows"
#define kParamRowsLabel "Rows"
#define kParamRowsHint "Rows in grid"
#define kParamRowsDefault 2

#define kParamCols "cols"
#define kParamColsLabel "Colums"
#define kParamColsHint "Columns in grid"
#define kParamColsDefault 2

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

using namespace OFX;

class MagickTilePlugin : public OFX::ImageEffect
{
public:
    MagickTilePlugin(OfxImageEffectHandle handle);
    virtual ~MagickTilePlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::IntParam *rows_;
    OFX::IntParam *cols_;
};

MagickTilePlugin::MagickTilePlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick("");
    MagickCore::MagickCoreGenesis( NULL, MagickCore::MagickTrue );
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
    OfxRectI srcRod,srcBounds;
    OFX::BitDepthEnum bitDepth = eBitDepthNone;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        bitDepth = srcImg->getPixelDepth();
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
    OfxRectI dstRod = dstImg->getRegionOfDefinition();

    // get bit depth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && dstBitDepth != srcImg->getPixelDepth())) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get pixel component
    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if ((dstComponents != OFX::ePixelComponentRGBA && dstComponents != OFX::ePixelComponentRGB && dstComponents != OFX::ePixelComponentAlpha) ||
        (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }
    std::string channels;
    switch (dstComponents) {
    case ePixelComponentRGBA:
        channels = "RGBA";
        break;
    case ePixelComponentRGB:
        channels = "RGB";
        break;
    case ePixelComponentAlpha:
        channels = "A";
        break;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // get param
    int rows,cols;
    rows_->getValueAtTime(args.time, rows);
    cols_->getValueAtTime(args.time, cols);

    // read image
    Magick::Image image(srcRod.x2-srcRod.x1,srcRod.y2-srcRod.y1,channels,Magick::FloatPixel,(float*)srcImg->getPixelData());

    // proc image
    int magickWidth = srcRod.x2-srcRod.x1;
    int magickHeight = srcRod.y2-srcRod.y1;
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

    Magick::Montage montageSettings;
    montageSettings.shadow(false);

	// avoid warn, set a default font
    std::string fontFile;
    char **fonts;
    std::size_t fontList;
    fonts=MagickCore::MagickQueryFonts("*",&fontList);
    fontFile = fonts[0];
    for (size_t i = 0; i < fontList; i++)
        free(fonts[i]);
    montageSettings.font(fontFile);

    montageSettings.backgroundColor(Magick::Color("rgba(0,0,0,0)"));
    montageSettings.geometry(thumb);
    montageSettings.tile(grid);

    std::list<Magick::Image> montagelist;
    std::list<Magick::Image> imageList;
    for(int y = 0; y < thumbs; y++)
        imageList.push_back(image);

    Magick::montageImages(&montagelist,imageList.begin(),imageList.end(),montageSettings);
    Magick::appendImages(&image,montagelist.begin(),montagelist.end());

    // check bit depth
    switch (bitDepth) {
    case OFX::eBitDepthUByte:
        if (image.depth()>8)
            image.depth(8);
        break;
    case OFX::eBitDepthUShort:
        if (image.depth()>16)
            image.depth(16);
        break;
    }

    // return image
    image.write(0,0,dstRod.x2-dstRod.x1,dstRod.y2-dstRod.y1,channels,Magick::FloatPixel,(float*)dstImg->getPixelData());
}

bool MagickTilePlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

    // add supported pixel depths
    //desc.addSupportedBitDepth(eBitDepthUByte); // not tested yet
    //desc.addSupportedBitDepth(eBitDepthUShort); // not tested yet
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
    //srcClip->addSupportedComponent(ePixelComponentAlpha); // should work, not tested
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->addSupportedComponent(ePixelComponentRGB);
    //dstClip->addSupportedComponent(ePixelComponentAlpha); // should work, not tested
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages and to things in
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
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
