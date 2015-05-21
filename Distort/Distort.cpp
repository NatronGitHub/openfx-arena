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

#include "Distort.h"

#include "ofxsMacros.h"
#include <Magick++.h>

#define kPluginName "Distort"
#define kPluginGrouping "Filter"
#define kPluginDescription  "Distort image using varius techniques. \n\nhttps://github.com/olear/openfx-arena"

#define kPluginIdentifier "net.fxarena.openfx.Distort"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kParamVPixel "virtualPixelMethod"
#define kParamVPixelLabel "Virtual Pixel"
#define kParamVPixelHint "Virtual Pixel Method"
#define kParamVPixelDefault 7

#define kParamDistort "distort"
#define kParamDistortLabel "Distort"
#define kParamDistortHint "Distort technique"
#define kParamDistortDefault 0

#define kParamArcAngle "arcAngle"
#define kParamArcAngleLabel "Angle"
#define kParamArcAngleHint "Arc angle"
#define kParamArcAngleDefault 60

#define kParamSwirl "swirlDegree"
#define kParamSwirlLabel "Degree"
#define kParamSwirlHint "Swirl image by degree"
#define kParamSwirlDefault 60

#define kParamImplode "implodeFactor"
#define kParamImplodeLabel "Factor"
#define kParamImplodeHint "Implode image by factor"
#define kParamImplodeDefault 0.5

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderInstanceSafe

using namespace OFX;

class DistortPlugin : public OFX::ImageEffect
{
public:
    DistortPlugin(OfxImageEffectHandle handle);
    virtual ~DistortPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::ChoiceParam *vpixel_;
    OFX::ChoiceParam *distort_;
    OFX::DoubleParam *arcAngle_;
    OFX::DoubleParam *swirlDegree_;
    OFX::DoubleParam *implode_;
};

DistortPlugin::DistortPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick("");
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    vpixel_ = fetchChoiceParam(kParamVPixel);
    distort_ = fetchChoiceParam(kParamDistort);
    arcAngle_ = fetchDoubleParam(kParamArcAngle);
    swirlDegree_ = fetchDoubleParam(kParamSwirl);
    implode_ = fetchDoubleParam(kParamImplode);
    assert(vpixel_ && distort_ && arcAngle_ && swirlDegree_ && implode_);
}

DistortPlugin::~DistortPlugin()
{
}

void DistortPlugin::render(const OFX::RenderArguments &args)
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

    // get params
    int vpixel,distort,value;
    double arcAngle,swirlDegree,implode;
    vpixel_->getValueAtTime(args.time, vpixel);
    distort_->getValueAtTime(args.time, distort);
    arcAngle_->getValueAtTime(args.time, arcAngle);
    swirlDegree_->getValueAtTime(args.time, swirlDegree);
    implode_->getValueAtTime(args.time, implode);

    value = 1; // TODO! should this be a param?

    // read image
    Magick::Image image(srcRod.x2-srcRod.x1,srcRod.y2-srcRod.y1,channels,Magick::FloatPixel,(float*)srcImg->getPixelData());

    // create empty container
    Magick::Image container(Magick::Geometry(srcRod.x2-srcRod.x1,srcRod.y2-srcRod.y1),Magick::Color("rgba(0,0,0,0)"));

    // dimension
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    int offsetX = 0;
    int offsetY = 0;

    // flip it, if needed
    if (distort==0||distort==1||distort==2)
        image.flip();

    // set virtual pixel
    switch (vpixel) {
    case 0:
        image.virtualPixelMethod(Magick::UndefinedVirtualPixelMethod);
        break;
    case 1:
        image.virtualPixelMethod(Magick::BackgroundVirtualPixelMethod);
        break;
    case 2:
        image.virtualPixelMethod(Magick::BlackVirtualPixelMethod);
        break;
    case 3:
        image.virtualPixelMethod(Magick::CheckerTileVirtualPixelMethod);
        break;
    case 4:
        image.virtualPixelMethod(Magick::DitherVirtualPixelMethod);
        break;
    case 5:
        image.virtualPixelMethod(Magick::EdgeVirtualPixelMethod);
        break;
    case 6:
        image.virtualPixelMethod(Magick::GrayVirtualPixelMethod);
        break;
    case 7:
        image.virtualPixelMethod(Magick::HorizontalTileVirtualPixelMethod);
        break;
    case 8:
        image.virtualPixelMethod(Magick::HorizontalTileEdgeVirtualPixelMethod);
        break;
    case 9:
        image.virtualPixelMethod(Magick::MirrorVirtualPixelMethod);
        break;
    case 10:
        image.virtualPixelMethod(Magick::RandomVirtualPixelMethod);
        break;
    case 11:
        image.virtualPixelMethod(Magick::TileVirtualPixelMethod);
        break;
    case 12:
        image.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
        break;
    case 13:
        image.virtualPixelMethod(Magick::VerticalTileVirtualPixelMethod);
        break;
    case 14:
        image.virtualPixelMethod(Magick::VerticalTileEdgeVirtualPixelMethod);
        break;
    case 15:
        image.virtualPixelMethod(Magick::WhiteVirtualPixelMethod);
        break;
    }

    // distort method
    double distortArgs[4];
    switch (distort) {
    case 0: // Polar Distort
        image.backgroundColor(Magick::Color("black")); // TODO! own param?
        image.distort(Magick::PolarDistortion, value, distortArgs, Magick::MagickTrue);
        if (image.rows()>height)
            image.scale(Magick::Geometry(NULL,height));
        if (image.columns()<width)
            offsetX = (width-image.columns())/2;
        break;
    case 1: // DePolar Distort
        image.backgroundColor(Magick::Color("black")); // TODO! own param?
        image.distort(Magick::DePolarDistortion, value, distortArgs, Magick::MagickTrue);
        if (image.columns()>width)
            image.scale(Magick::Geometry(width,NULL));
        if (image.rows()>height)
            image.scale(Magick::Geometry(NULL,height));
        if (image.rows()<height)
            offsetY = (height-image.rows())/2;
        if (image.columns()<width)
            offsetX = (width-image.columns())/2;
        break;
    case 2: // Arc Distort
        distortArgs[0] = arcAngle;
        image.backgroundColor(Magick::Color("black")); // TODO! own param?
        image.distort(Magick::ArcDistortion, value, distortArgs, Magick::MagickTrue);
        if (image.columns()>width)
            image.scale(Magick::Geometry(width,NULL));
        if (image.rows()>height)
            image.scale(Magick::Geometry(NULL,height));
        if (image.rows()<height)
            offsetY = (height-image.rows())/2;
        if (image.columns()<width)
            offsetX = (width-image.columns())/2;
        break;
    case 3: // Swirl
        if (swirlDegree!=0)
            image.swirl(swirlDegree);
        break;
    case 4: // Implode
        if (implode!=0)
            image.implode(implode);
        break;
    }

    // flip and comp, if needed
    if (distort==0||distort==1||distort==2) {
        image.flip();
        container.composite(image,offsetX,offsetY,Magick::OverCompositeOp);
    }
    else
        container=image;

    // return image
    switch (dstBitDepth) {
    case eBitDepthUByte:
        if (container.depth()>8)
            container.depth(8);
        container.write(0,0,width,height,channels,Magick::CharPixel,(float*)dstImg->getPixelData());
        break;
    case eBitDepthUShort:
        if (container.depth()>16)
            container.depth(16);
        container.write(0,0,width,height,channels,Magick::ShortPixel,(float*)dstImg->getPixelData());
        break;
    case eBitDepthFloat:
        container.write(0,0,width,height,channels,Magick::FloatPixel,(float*)dstImg->getPixelData());
        break;
    }
}

bool DistortPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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

mDeclarePluginFactory(DistortPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void DistortPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
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
void DistortPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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
    PageParamDescriptor *pagePolar = desc.definePageParam("(De)Polar");
    PageParamDescriptor *pageArc = desc.definePageParam("Arc");
    PageParamDescriptor *pageSwirl = desc.definePageParam("Swirl");
    PageParamDescriptor *pageImplode = desc.definePageParam("Implode");
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamVPixel);
        param->setLabel(kParamVPixelLabel);
        param->setHint(kParamVPixelHint);

        param->appendOption("Undefined");
        param->appendOption("Background");
        param->appendOption("Black");
        param->appendOption("CheckerTile");
        param->appendOption("Dither");
        param->appendOption("Edge");
        param->appendOption("Gray");
        param->appendOption("HorizontalTile");
        param->appendOption("HorizontalTileEdge");
        param->appendOption("Mirror");
        param->appendOption("Random");
        param->appendOption("Tile");
        param->appendOption("Transparent");
        param->appendOption("VerticalTile");
        param->appendOption("VerticalTileEdge");
        param->appendOption("White");

        param->setDefault(kParamVPixelDefault);

        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamDistort);
        param->setLabel(kParamDistortLabel);
        param->setHint(kParamDistortHint);

        param->appendOption("Polar");
        param->appendOption("DePolar");
        param->appendOption("Arc");
        param->appendOption("Swirl");
        param->appendOption("Implode");

        param->setDefault(kParamDistortDefault);

        param->setAnimates(true);
        page->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcAngle);
        param->setLabel(kParamArcAngleLabel);
        param->setHint(kParamArcAngleHint);
        param->setRange(1, 360);
        param->setDisplayRange(1, 360);
        param->setDefault(kParamArcAngleDefault);
        pageArc->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSwirl);
        param->setLabel(kParamSwirlLabel);
        param->setHint(kParamSwirlHint);
        param->setRange(-360, 360);
        param->setDisplayRange(-360, 360);
        param->setDefault(kParamSwirlDefault);
        pageSwirl->addChild(*param);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamImplode);
        param->setLabel(kParamImplodeLabel);
        param->setHint(kParamImplodeHint);
        param->setRange(-10, 1.5);
        param->setDisplayRange(-10, 1.5);
        param->setDefault(kParamImplodeDefault);
        pageImplode->addChild(*param);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* DistortPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new DistortPlugin(handle);
}

void getDistortPluginID(OFX::PluginFactoryArray &ids)
{
    static DistortPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
