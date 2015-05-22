/*

openfx-arena - https://github.com/olear/openfx-arena

Copyright (c) 2015, Ole-André Rodlie <olear@fxarena.net>
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Neither the name of FxArena DA nor the names of its
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

#include "Mirror.h"

#include "ofxsMacros.h"
#include <Magick++.h>
#include <iostream>

#define kPluginName "Mirror"
#define kPluginGrouping "Filter"
#define kPluginDescription  "Mirrors, tiles and reflect image in various ways. \n\nhttps://github.com/olear/openfx-arena"

#define kPluginIdentifier "net.fxarena.openfx.Mirror"
#define kPluginVersionMajor 2
#define kPluginVersionMinor 0

#define kParamMirror "mirrorType"
#define kParamMirrorLabel "Type"
#define kParamMirrorHint "Select mirror type"

#define kParamRows "rows"
#define kParamRowsLabel "Rows"
#define kParamRowsHint "Rows in grid"
#define kParamRowsDefault 2

#define kParamCols "cols"
#define kParamColsLabel "Colums"
#define kParamColsHint "Columns in grid"
#define kParamColsDefault 2

#define kParamSpace "spacing"
#define kParamSpaceLabel "Space"
#define kParamSpaceHint "Space between image and reflection"
#define kParamSpaceDefault 0

#define kParamOffset "offset"
#define kParamOffsetLabel "Offset"
#define kParamOffsetHint "Mirror offset"
#define kParamOffsetDefault 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 0
#define kRenderThreadSafety eRenderInstanceSafe

using namespace OFX;

class MirrorPlugin : public OFX::ImageEffect
{
public:
    MirrorPlugin(OfxImageEffectHandle handle);
    virtual ~MirrorPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::Clip *maskClip_;
    OFX::ChoiceParam *mirror_;
    OFX::IntParam *rows_;
    OFX::IntParam *cols_;
    OFX::IntParam *spacing_;
    OFX::IntParam *offset_;
};

MirrorPlugin::MirrorPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
, maskClip_(0)
{
    Magick::InitializeMagick(NULL);
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    maskClip_ = getContext() == OFX::eContextFilter ? NULL : fetchClip(getContext() == OFX::eContextPaint ? "Brush" : "Mask");
    assert(!maskClip_ || maskClip_->getPixelComponents() == OFX::ePixelComponentAlpha);

    mirror_ = fetchChoiceParam(kParamMirror);
    rows_ = fetchIntParam(kParamRows);
    cols_ = fetchIntParam(kParamCols);
    spacing_ = fetchIntParam(kParamSpace);
    offset_ = fetchIntParam(kParamOffset);
    assert(mirror_ && rows_ && cols_ && spacing_ && offset_);
}

MirrorPlugin::~MirrorPlugin()
{
}

void MirrorPlugin::render(const OFX::RenderArguments &args)
{
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
    int mirror,rows,cols,spacing,offset;;
    rows_->getValueAtTime(args.time, rows);
    cols_->getValueAtTime(args.time, cols);
    mirror_->getValueAtTime(args.time, mirror);
    spacing_->getValueAtTime(args.time, spacing);
    offset_->getValueAtTime(args.time, offset);

    bool use_tile = false;
    bool use_refl = false;

    // read image
    Magick::Image image(srcRod.x2-srcRod.x1,srcRod.y2-srcRod.y1,channels,Magick::FloatPixel,(float*)srcImg->getPixelData());

    // proc image
    int magickWidth = srcRod.x2-srcRod.x1;
    int magickHeight = srcRod.y2-srcRod.y1;
    int mirrorWidth = magickWidth/2;
    int mirrorHeight = magickHeight/2;
    Magick::Image image1;
    Magick::Image image2;
    Magick::Image image3;
    Magick::Image image4;
    image1 = image;
    switch(mirror) {
    case 1: // North
          image1.flip();
          image1.crop(Magick::Geometry(magickWidth,mirrorHeight,0,0));
          break;
    case 2: // South
          image.flip();
          image1.crop(Magick::Geometry(magickWidth,mirrorHeight,0,0));
          break;
    case 3: // East
          image1.flop();
          image1.crop(Magick::Geometry(mirrorWidth,magickHeight,0,0));
          break;
    case 4: // West
        image.flop();
        image1.crop(Magick::Geometry(mirrorWidth,magickHeight,0,0));
          break;
    case 5: // NorthWest
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,0,mirrorHeight));
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 6: // NorthEast
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,mirrorWidth,mirrorHeight));
        image1.flop();
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 7: // SouthWest
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,0,0));
        image1.flip();
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 8: // SouthEast
        image1.crop(Magick::Geometry(mirrorWidth,mirrorHeight,mirrorWidth,0));
        image1.flop();
        image1.flip();
        image2 = image1;
        image2.flop();
        image3 = image2;
        image3.flip();
        image4 = image3;
        image4.flop();
        break;
    case 9: // Flip
        image.flip();
        break;
    case 10: // Flop
        image.flop();
        break;
    case 11: // Flip+Flop
        image.flip();
        image.flop();
        break;
    case 12: // Tile
        use_tile = true;
        break;
    case 13: // Reflection
        use_refl = true;
        break;
    }
    if (mirror==5||mirror==6||mirror==7||mirror==8) {
        image.composite(image1,0,mirrorHeight,Magick::OverCompositeOp);
        image.composite(image2,mirrorWidth,mirrorHeight,Magick::OverCompositeOp);
        image.composite(image3,mirrorWidth,0,Magick::OverCompositeOp);
        image.composite(image4,0,0,Magick::OverCompositeOp);
    }
    else {
        if (mirror==1||mirror==2||mirror==3||mirror==4)
            image.composite(image1,0,0,Magick::OverCompositeOp);
    }

    // Tile image, if enabled
    if (use_tile) {
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
    }

    // Reflection
    if (use_refl) {
        Magick::Image container(Magick::Geometry(magickWidth,magickHeight),Magick::Color("rgba(0,0,0,0)"));
        image1 = image;
        image1.flip();
        image1.crop(Magick::Geometry(magickWidth,mirrorHeight-offset,0,offset+offset));
        image.crop(Magick::Geometry(magickWidth,mirrorHeight+offset,0,mirrorHeight-offset));

        // apply mask if exists
        if (maskClip_ && maskClip_->isConnected()) {
            int maskWidth = maskRod.x2-maskRod.x1;
            int maskHeight = maskRod.y2-maskRod.y1;
            if (maskWidth>0&&maskHeight>0) {
                Magick::Image mask(maskWidth,maskHeight,"A",Magick::FloatPixel,(float*)maskImg->getPixelData());
                image1.composite(mask,0,0,Magick::CopyOpacityCompositeOp);
            }
        }

        container.composite(image1,0,-spacing,Magick::OverCompositeOp);
        container.composite(image,0,mirrorHeight-offset,Magick::OverCompositeOp);
        image=container;
    }

    // return image
    switch (dstBitDepth) {
    case eBitDepthUByte:
        if (image.depth()>8)
            image.depth(8);
        image.write(0,0,magickWidth,magickHeight,channels,Magick::CharPixel,(float*)dstImg->getPixelData());
        break;
    case eBitDepthUShort:
        if (image.depth()>16)
            image.depth(16);
        image.write(0,0,magickWidth,magickHeight,channels,Magick::ShortPixel,(float*)dstImg->getPixelData());
        break;
    case eBitDepthFloat:
        image.write(0,0,magickWidth,magickHeight,channels,Magick::FloatPixel,(float*)dstImg->getPixelData());
        break;
    }
}

bool MirrorPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(MirrorPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void MirrorPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthUByte);
    desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void MirrorPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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

    // create optional mask clip
    ClipDescriptor *maskClip = desc.defineClip("Mask");
    maskClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    maskClip->setTemporalClipAccess(false);
    maskClip->setOptional(true);
    maskClip->setSupportsTiles(kSupportsTiles);
    maskClip->setIsMask(true);

    // make some pages and to things in
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    PageParamDescriptor *pageTile = desc.definePageParam("Tile");
    PageParamDescriptor *pageRefl = desc.definePageParam("Reflection");
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamMirror);
        param->setLabel(kParamMirrorLabel);
        param->setHint(kParamMirrorHint);
        param->appendOption("None");
        param->appendOption("North");
        param->appendOption("South");
        param->appendOption("East");
        param->appendOption("West");
        param->appendOption("NorthWest");
        param->appendOption("NorthEast");
        param->appendOption("SouthWest");
        param->appendOption("SouthEast");
        param->appendOption("Flip");
        param->appendOption("Flop");
        param->appendOption("Flip+Flop");
        param->appendOption("Tile");
        param->appendOption("Reflection");
        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamRows);
        param->setLabel(kParamRowsLabel);
        param->setHint(kParamRowsHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamRowsDefault);
        pageTile->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamCols);
        param->setLabel(kParamColsLabel);
        param->setHint(kParamColsHint);
        param->setRange(1, 100);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamColsDefault);
        pageTile->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamOffset);
        param->setLabel(kParamOffsetLabel);
        param->setHint(kParamOffsetHint);
        param->setRange(0, 500);
        param->setDisplayRange(0, 50);
        param->setDefault(kParamOffsetDefault);
        pageRefl->addChild(*param);
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamSpace);
        param->setLabel(kParamSpaceLabel);
        param->setHint(kParamSpaceHint);
        param->setRange(0, 100);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamSpaceDefault);
        pageRefl->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* MirrorPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new MirrorPlugin(handle);
}

void getMirrorPluginID(OFX::PluginFactoryArray &ids)
{
    static MirrorPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
