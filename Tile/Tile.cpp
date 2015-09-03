/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "Tile.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include <Magick++.h>
#include <iostream>

#define kPluginName "TileOFX"
#define kPluginGrouping "Transform"
#define kPluginIdentifier "net.fxarena.openfx.Tile"
#define kPluginVersionMajor 3
#define kPluginVersionMinor 1
#define kPluginMagickVersion 26640

#define kParamRows "rows"
#define kParamRowsLabel "Rows"
#define kParamRowsHint "Rows in grid"
#define kParamRowsDefault 2

#define kParamCols "cols"
#define kParamColsLabel "Colums"
#define kParamColsHint "Columns in grid"
#define kParamColsDefault 2

#define kParamTileTimeOffset "offset"
#define kParamTileTimeOffsetLabel "Time Offset"
#define kParamTileTimeOffsetHint "Set a time offset"
#define kParamTileTimeOffsetDefault 0

#define kParamTileTimeOffsetFirst "keepFirst"
#define kParamTileTimeOffsetFirstLabel "Keep first frame"
#define kParamTileTimeOffsetFirstHint "Stay on first frame if offset"
#define kParamTileTimeOffsetFirstDefault true

#define kParamMatte "matte"
#define kParamMatteLabel "Matte"
#define kParamMatteHint "Merge Alpha before applying effect"
#define kParamMatteDefault false

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

using namespace OFX;

class TilePlugin : public OFX::ImageEffect
{
public:
    TilePlugin(OfxImageEffectHandle handle);
    virtual ~TilePlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::Clip *maskClip_;
    OFX::IntParam *rows_;
    OFX::IntParam *cols_;
    OFX::IntParam *offset_;
    OFX::BooleanParam *firstFrame_;
    OFX::BooleanParam *matte_;
};

TilePlugin::TilePlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick(NULL);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    assert(!maskClip_ || maskClip_->getPixelComponents() == OFX::ePixelComponentAlpha);

    rows_ = fetchIntParam(kParamRows);
    cols_ = fetchIntParam(kParamCols);
    offset_ = fetchIntParam(kParamTileTimeOffset);
    firstFrame_ = fetchBooleanParam(kParamTileTimeOffsetFirst);
    matte_ = fetchBooleanParam(kParamMatte);

    assert(rows_ && cols_ && offset_ && firstFrame_ && matte_);
}

TilePlugin::~TilePlugin()
{
}

void TilePlugin::render(const OFX::RenderArguments &args)
{
    // render scale
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get src clip
    if (!srcClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(srcClip_);
    std::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        if (srcImg->getRenderScale().x != args.renderScale.x ||
            srcImg->getRenderScale().y != args.renderScale.y ||
            srcImg->getField() != args.fieldToRender) {
            setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
            OFX::throwSuiteStatusException(kOfxStatFailed);
            return;
        }
    }

    // Get mask clip
    std::auto_ptr<const OFX::Image> maskImg((getContext() != OFX::eContextFilter && maskClip_ && maskClip_->isConnected()) ? maskClip_->fetchImage(args.time) : 0);
    OfxRectI maskRod;
    if (getContext() != OFX::eContextFilter && maskClip_ && maskClip_->isConnected())
        maskRod=maskImg->getRegionOfDefinition();

    // get dest clip
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

    // get bit depth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && (dstBitDepth != srcImg->getPixelDepth()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get pixel component
    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != OFX::ePixelComponentRGBA || (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds?
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // get params
    int rows = 0;
    int cols = 0;
    int offset = 0;
    bool firstFrame = false;
    bool matte = false;
    rows_->getValueAtTime(args.time, rows);
    cols_->getValueAtTime(args.time, cols);
    offset_->getValueAtTime(args.time, offset);
    firstFrame_->getValueAtTime(args.time, firstFrame);
    matte_->getValueAtTime(args.time, matte);

    // setup
    int srcWidth = srcRod.x2-srcRod.x1;
    int srcHeight = srcRod.y2-srcRod.y1;
    int tileWidth = srcWidth/rows;
    int tileHeight = srcHeight/cols;
    int thumbs = rows*cols;
    std::string thumb;
    std::ostringstream makeThumb;
    makeThumb << tileWidth << "x" << tileHeight << "-0-0";
    thumb = makeThumb.str();
    std::string grid;
    std::ostringstream makeGrid;
    makeGrid << rows << "x" << cols;
    grid = makeGrid.str();
    std::list<Magick::Image> montagelist;
    std::list<Magick::Image> imageList;
    Magick::Image image;
    Magick::Montage montage;

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    // read source image
    Magick::Image container(Magick::Geometry(srcWidth,srcHeight),Magick::Color("rgba(0,0,0,0)"));
    if (srcClip_ && srcClip_->isConnected())
        image.read(srcWidth,srcHeight,"RGBA",Magick::FloatPixel,(float*)srcImg->getPixelData());

    #ifdef DEBUG
    image.debug(true);
    #endif

    // apply mask
    if (maskClip_ && maskClip_->isConnected()) {
        int maskWidth = maskRod.x2-maskRod.x1;
        int maskHeight = maskRod.y2-maskRod.y1;
        if (maskWidth>0 && maskHeight>0) {
            Magick::Image mask(maskWidth,maskHeight,"A",Magick::FloatPixel,(float*)maskImg->getPixelData());
            image.composite(mask,0,0,Magick::CopyOpacityCompositeOp);
            Magick::Image container(Magick::Geometry(srcWidth,srcHeight),Magick::Color("rgba(0,0,0,0)"));
            container.composite(image,0,0,Magick::OverCompositeOp);
            image=container;
        }
    }

    // setup montage
    std::string fontFile;
    char **fonts;
    std::size_t fontList;
    fonts=MagickCore::MagickQueryFonts("*",&fontList);
    fontFile = fonts[0];
    for (size_t i = 0; i < fontList; i++)
        free(fonts[i]);

    if (!fontFile.empty())
        montage.font(fontFile); // avoid warn, set default font
    else // broken installation
        setPersistentMessage(OFX::Message::eMessageError, "", "No fonts found, please check installation");
    montage.shadow(false);
    montage.backgroundColor(Magick::Color("rgba(0,0,0,0)"));
    montage.geometry(thumb);
    montage.tile(grid);

    if (matte) {
        image.matte(false);
        image.matte(true);
    }

    // add images
    if (offset==0) {
        for(int y = 0; y < thumbs; y++)
            imageList.push_back(image);
    }
    else { // time offset
        int counter;
        if (firstFrame) {
            imageList.push_back(image);
            counter=thumbs-1;
        }
        else
            counter=thumbs;
        int frame = args.time+offset;
        for(int y = 0; y < counter; y++) {
            std::auto_ptr<const OFX::Image> tileImg(srcClip_->fetchImage(frame));
            if (tileImg.get()) {
                OfxRectI tileRod;
                tileRod = tileImg->getRegionOfDefinition();
                int tileWidth = tileRod.x2-tileRod.x1;
                int tileHeight = tileRod.y2-tileRod.y1;
                if (tileWidth>0&&tileHeight>0) {
                    Magick::Image tmpTile(tileWidth,tileHeight,"RGBA",Magick::FloatPixel,(float*)tileImg->getPixelData());
                    std::size_t tileColumns = tileWidth;
                    std::size_t tileRows = tileHeight;
                    if (tmpTile.columns()==tileColumns && tmpTile.rows()==tileRows)
                        imageList.push_back(tmpTile);
                }
            }
            frame++;
        }
    }

    // do a montage
    Magick::montageImages(&montagelist,imageList.begin(),imageList.end(),montage);

    // append images to container
    Magick::appendImages(&container,montagelist.begin(),montagelist.end());

    // return image
    if (dstClip_ && dstClip_->isConnected() && srcClip_ && srcClip_->isConnected())
        container.write(0,0,srcWidth,srcHeight,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
}

bool TilePlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(TilePluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void TilePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    if (magickNumber != kPluginMagickVersion)
        magickString.append("\n\nWarning! You are using an unsupported version of ImageMagick.");
    desc.setPluginDescription("Tile transform node.\n\nPowered by "+magickString);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TilePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create optional mask clip
    ClipDescriptor *maskClip = desc.defineClip("Mask");
    maskClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    maskClip->setTemporalClipAccess(false);
    maskClip->setOptional(true);
    maskClip->setSupportsTiles(kSupportsTiles);
    maskClip->setIsMask(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make pages and params
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamRows);
        param->setLabel(kParamRowsLabel);
        param->setHint(kParamRowsHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamRowsDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamCols);
        param->setLabel(kParamColsLabel);
        param->setHint(kParamColsHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamColsDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamTileTimeOffset);
        param->setLabel(kParamTileTimeOffsetLabel);
        param->setHint(kParamTileTimeOffsetHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 100);
        param->setDefault(kParamTileTimeOffsetDefault);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamTileTimeOffsetFirst);
        param->setLabel(kParamTileTimeOffsetFirstLabel);
        param->setHint(kParamTileTimeOffsetFirstHint);
        param->setAnimates(true);
        param->setDefault(kParamTileTimeOffsetFirstDefault);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamMatte);
        param->setLabel(kParamMatteLabel);
        param->setHint(kParamMatteHint);
        param->setDefault(kParamMatteDefault);
        param->setAnimates(true);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TilePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TilePlugin(handle);
}

void getTilePluginID(OFX::PluginFactoryArray &ids)
{
    static TilePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
