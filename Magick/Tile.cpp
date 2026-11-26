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

#include "MagickCommon.h"
#include "PackedImage.h"

#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include "ofxsImageEffect.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#define kPluginName "TileOFX"
#define kPluginGrouping "Extra/Transform"
#define kPluginIdentifier "net.fxarena.openfx.Tile"
#define kPluginVersionMajor 4 // 4: unpremultiply the source before ImageMagick (see PackedImage::PackedSrc)
#define kPluginVersionMinor 0

#define kParamRows "rows"
#define kParamRowsLabel "Rows"
#define kParamRowsHint "Rows in grid"
#define kParamRowsDefault 2
#define kParamRowsMax 100

#define kParamCols "cols"
#define kParamColsLabel "Columns"
#define kParamColsHint "Columns in grid"
#define kParamColsDefault 2
#define kParamColsMax 100

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
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false

#define kParamOpenMP "openmp"
#define kParamOpenMPLabel "OpenMP"
#define kParamOpenMPHint "Kept for old projects; ImageMagick runs on one thread unless the MAGICK_THREAD_LIMIT environment variable is set."
#define kParamOpenMPDefault false

using namespace OFX;

OFXS_NAMESPACE_ANONYMOUS_ENTER

static bool _hasOpenMP = false;

class TilePlugin : public OFX::ImageEffect
{
public:
    TilePlugin(OfxImageEffectHandle handle, bool unpremultSrc);
    virtual ~TilePlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
    virtual void getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames) OVERRIDE FINAL;
private:
    bool unpremultSrc_; // false in versions released before the premultiplication fix
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::IntParam *rows_;
    OFX::IntParam *cols_;
    OFX::IntParam *offset_;
    OFX::BooleanParam *firstFrame_;
    OFX::BooleanParam *matte_;
    OFX::BooleanParam *enableOpenMP_;
};

TilePlugin::TilePlugin(OfxImageEffectHandle handle, bool unpremultSrc)
: OFX::ImageEffect(handle)
, unpremultSrc_(unpremultSrc)
, dstClip_(NULL)
, srcClip_(NULL)
{
    magickInitialize();
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    rows_ = fetchIntParam(kParamRows);
    cols_ = fetchIntParam(kParamCols);
    offset_ = fetchIntParam(kParamTileTimeOffset);
    firstFrame_ = fetchBooleanParam(kParamTileTimeOffsetFirst);
    matte_ = fetchBooleanParam(kParamMatte);
    enableOpenMP_ = fetchBooleanParam(kParamOpenMP);

    assert(rows_ && cols_ && offset_ && firstFrame_ && matte_ && enableOpenMP_);
}

TilePlugin::~TilePlugin()
{
}

// With a Time Offset, render() also fetches the frames of the other tiles: declare them, since
// without temporal clip access the host assumes that only the current frame is needed
void TilePlugin::getFramesNeeded(const OFX::FramesNeededArguments &args, OFX::FramesNeededSetter &frames)
{
    OfxRangeD range;
    range.min = range.max = args.time;
    frames.setFramesNeeded(*srcClip_, range);
    const int offset = offset_->getValueAtTime(args.time);
    if (offset != 0) {
        // as in render()
        const int rows = std::min(std::max(rows_->getValueAtTime(args.time), 1), kParamRowsMax);
        const int cols = std::min(std::max(cols_->getValueAtTime(args.time), 1), kParamColsMax);
        const int counter = firstFrame_->getValueAtTime(args.time) ? rows * cols - 1 : rows * cols;
        if (counter > 0) {
            range.min = std::floor(args.time) + offset;
            range.max = range.min + (counter - 1);
            frames.setFramesNeeded(*srcClip_, range);
        }
    }
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
    OFX::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        checkBadRenderScaleOrField(srcImg, args);
    } else {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // get dest clip
    if (!dstClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);
    OFX::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    checkBadRenderScaleOrField(dstImg, args);

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
    // the OFX spec doesn't make the host clamp to the params' range (1..100): a script or a
    // project may still give 0 or less, and the divisions below would then crash (SIGFPE), or a
    // huge value, which would overflow rows*cols
    rows = std::min(std::max(rows, 1), kParamRowsMax);
    cols = std::min(std::max(cols, 1), kParamColsMax);

    // setup
    int srcWidth = srcRod.x2-srcRod.x1;
    int srcHeight = srcRod.y2-srcRod.y1;
    // at least 1 pixel: ImageMagick reads a 0 width or height in the thumbnail geometry as
    // "keep the original size"
    int tileWidth = std::max(srcWidth/rows, 1);
    int tileHeight = std::max(srcHeight/cols, 1);
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

    // read source image
    Magick::Image container(Magick::Geometry(srcWidth,srcHeight),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(srcWidth,srcHeight),Magick::Color("rgba(0,0,0,1)"));
    if (srcClip_ && srcClip_->isConnected()) {
        // ImageMagick expects straight colour. With Matte, alpha is discarded and the
        // premultiplied colour is the intended "flattened over black" input.
        PackedImage::PackedSrc srcPixels(srcImg.get(), srcRod,
                                         unpremultSrc_ && !matte && srcImg->getPreMultiplication() == OFX::eImagePreMultiplied);
        image.read(srcWidth,srcHeight,"RGBA",Magick::FloatPixel,srcPixels.data());
    }

    // setup montage
    std::string fontFile;
    std::size_t fontList = 0;
    char **fonts = MagickCore::MagickQueryFonts("*",&fontList);
    // MagickQueryFonts returns NULL (or an empty list) if no font is found
    if (fonts) {
        if (fontList > 0 && fonts[0]) {
            fontFile = fonts[0];
        }
        for (size_t i = 0; i < fontList; i++) {
            MagickCore::MagickRelinquishMemory(fonts[i]);
        }
        MagickCore::MagickRelinquishMemory(fonts);
    }

    if (!fontFile.empty())
        montage.font(fontFile); // avoid warn, set default font
    else // broken installation
        setPersistentMessage(OFX::Message::eMessageError, "", "No fonts found, please check installation");
    montage.shadow(false);
    montage.backgroundColor(Magick::Color("rgba(0,0,0,0)"));
    montage.geometry(thumb);
    montage.tile(grid);

    if (matte) {
#if MagickLibVersion >= 0x700
        image.alpha(false);
        image.alpha(true);
#else
        image.matte(false);
        image.matte(true);
#endif
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
        double frame = std::floor(args.time) + offset; // not truncated towards zero
        for(int y = 0; y < counter; y++) {
            OFX::auto_ptr<const OFX::Image> tileImg(srcClip_->fetchImage(frame));
            if (tileImg.get()) {
                OfxRectI tileRod;
                tileRod = tileImg->getRegionOfDefinition();
                int tileWidth = tileRod.x2-tileRod.x1;
                int tileHeight = tileRod.y2-tileRod.y1;
                if (tileWidth>0&&tileHeight>0) {
                    // Matte only applies to the current frame: always unpremultiply here
                    PackedImage::PackedSrc tilePixels(tileImg.get(), tileRod,
                                                      unpremultSrc_ && tileImg->getPreMultiplication() == OFX::eImagePreMultiplied);
                    Magick::Image tmpTile(tileWidth,tileHeight,"RGBA",Magick::FloatPixel,tilePixels.data());
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
    if (dstClip_ && dstClip_->isConnected()) {
        output.composite(container, 0, 0, Magick::OverCompositeOp);
#if MagickLibVersion >= 0x700
        output.composite(container, 0, 0, Magick::CopyAlphaCompositeOp);
#else
        output.composite(container, 0, 0, Magick::CopyOpacityCompositeOp);
#endif
        PackedImage::PackedDst dstPixels(dstImg.get(), args.renderWindow);
        output.write(0,0,args.renderWindow.x2 - args.renderWindow.x1,args.renderWindow.y2 - args.renderWindow.y1,"RGBA",Magick::FloatPixel,dstPixels.data());
        dstPixels.commit();
    }
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

mDeclarePluginFactoryVersioned(TilePluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
template<unsigned int majorVersion>
void TilePluginFactory<majorVersion>::describe(OFX::ImageEffectDescriptor &desc)
{
    if (majorVersion < kPluginVersionMajor) {
        desc.setIsObsolete(true);
    }
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("Tile transform node.");

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
    desc.setTemporalClipAccess(true); // with a Time Offset, see getFramesNeeded()
    desc.setHostMaskingEnabled(true);
    desc.setHostMixingEnabled(true);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
template<unsigned int majorVersion>
void TilePluginFactory<majorVersion>::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    std::string features = MagickCore::GetMagickFeatures();
    if (features.find("OpenMP") != std::string::npos)
        _hasOpenMP = true;

    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(true); // with a Time Offset, see getFramesNeeded()
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make pages and params
    PageParamDescriptor *page = desc.definePageParam("Controls");
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamRows);
        param->setLabel(kParamRowsLabel);
        param->setHint(kParamRowsHint);
        param->setRange(1, kParamRowsMax);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamRowsDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamCols);
        param->setLabel(kParamColsLabel);
        param->setHint(kParamColsHint);
        param->setRange(1, kParamColsMax);
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
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamOpenMP);
        param->setLabel(kParamOpenMPLabel);
        param->setHint(kParamOpenMPHint);
        param->setDefault(kParamOpenMPDefault);
        param->setIsSecret(true);
        param->setAnimates(false);
        if (!_hasOpenMP)
            param->setEnabled(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
template<unsigned int majorVersion>
ImageEffect* TilePluginFactory<majorVersion>::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    // versions before 4 fed premultiplied pixels to ImageMagick: keep their output
    return new TilePlugin(handle, majorVersion >= 4);
}

static TilePluginFactory<kPluginVersionMajor> p(kPluginIdentifier, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
// version 3, kept so that existing projects render as before
static TilePluginFactory<3> p3(kPluginIdentifier, 2);
mRegisterPluginFactoryInstance(p3)

OFXS_NAMESPACE_ANONYMOUS_EXIT
