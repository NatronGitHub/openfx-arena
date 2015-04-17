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

#include "IOUtility.h"
#include "ofxNatron.h"
#include "ofxsMacros.h"
#include <OpenImageIO/imageio.h>
#include <fontconfig/fontconfig.h>
#include <Magick++.h>

#define OPENIMAGEIO_THREAD_H
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#define kPluginName "MagickText"
#define kPluginGrouping "Draw"
#define kPluginDescription  "Use ImageMagick to write text on images."

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

#define kParamTextColor "textColor"
#define kParamTextColorLabel "Color"
#define kParamTextColorHint "The color of the text to render"

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

    OFX::Double2DParam *position_;
    OFX::StringParam *text_;
    OFX::IntParam *fontSize_;
    OFX::ChoiceParam *fontName_;
    OFX::RGBAParam *textColor_;
};

MagickTextPlugin::MagickTextPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    Magick::InitializeMagick("");

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && (srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA || srcClip_->getPixelComponents() == OFX::ePixelComponentRGB));

    position_ = fetchDouble2DParam(kParamPosition);
    text_ = fetchStringParam(kParamText);
    fontSize_ = fetchIntParam(kParamFontSize);
    fontName_ = fetchChoiceParam(kParamFontName);
    textColor_ = fetchRGBAParam(kParamTextColor);
    assert(position_ && text_ && fontSize_ && fontName_ && textColor_);
}

MagickTextPlugin::~MagickTextPlugin()
{
}

static OIIO::ImageSpec
imageSpecFromOFXImage(const OfxRectI &rod, const OfxRectI &bounds, OFX::PixelComponentEnum pixelComponents, OFX::BitDepthEnum bitDepth)
{
    OIIO::TypeDesc format;
 	switch (bitDepth) {
		case OFX::eBitDepthUByte:
			format = OIIO::TypeDesc::UINT8;
			break;
		case OFX::eBitDepthUShort:
			format = OIIO::TypeDesc::UINT16;
			break;
		case OFX::eBitDepthHalf:
			format = OIIO::TypeDesc::HALF;
			break;
		case OFX::eBitDepthFloat:
			format = OIIO::TypeDesc::FLOAT;
			break;
		default:
            throwSuiteStatusException(kOfxStatErrFormat);
			break;
	}
    int nchannels = 0, alpha_channel = -1;
    switch (pixelComponents) {
        case OFX::ePixelComponentAlpha:
            nchannels = 1;
            alpha_channel = 0;
            break;
        case OFX::ePixelComponentRGB:
            nchannels = 3;
            break;
        case OFX::ePixelComponentRGBA:
            nchannels = 4;
            alpha_channel = 3;
            break;
        default:
            throwSuiteStatusException(kOfxStatErrFormat);
            break;
    }
    OIIO::ImageSpec spec (format);
    spec.x = bounds.x1;
    spec.y = rod.y2 - bounds.y2;
    spec.width = bounds.x2 - bounds.x1;
    spec.height = bounds.y2 - bounds.y1;
    spec.full_x = rod.x1;
    spec.full_y = 0;
    spec.full_width = rod.x2 - rod.x1;
    spec.full_height = rod.y2 - rod.y1;
    spec.nchannels = nchannels;
    spec.alpha_channel = alpha_channel;
    return spec;
}

/* Override the render */
void
MagickTextPlugin::render(const OFX::RenderArguments &args)
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

    OfxRectI srcRod;
    OfxRectI srcBounds;
    OFX::PixelComponentEnum pixelComponents = ePixelComponentNone;
    int pixelComponentCount = 0;
    OFX::BitDepthEnum bitDepth = eBitDepthNone;
    OIIO::ImageSpec srcSpec;
    std::auto_ptr<const OIIO::ImageBuf> srcBuf;
    OfxRectI dstRod = dstImg->getRegionOfDefinition();
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        pixelComponents = srcImg->getPixelComponents();
        pixelComponentCount = srcImg->getPixelComponentCount();
        bitDepth = srcImg->getPixelDepth();
        srcSpec = imageSpecFromOFXImage(srcRod, srcBounds, pixelComponents, bitDepth);
        srcBuf.reset(new OIIO::ImageBuf("src", srcSpec, const_cast<void*>(srcImg->getPixelData())));

        if (!kSupportsTiles) {
            // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
            //  If a clip or plugin does not support tiled images, then the host should supply full RoD images to the effect whenever it fetches one.
            assert(srcRod.x1 == srcBounds.x1);
            assert(srcRod.x2 == srcBounds.x2);
            assert(srcRod.y1 == srcBounds.y1);
            assert(srcRod.y2 == srcBounds.y2); // crashes on Natron if kSupportsTiles=0 & kSupportsMultiResolution=1
            assert(dstRod.x1 == dstBounds.x1);
            assert(dstRod.x2 == dstBounds.x2);
            assert(dstRod.y1 == dstBounds.y1);
            assert(dstRod.y2 == dstBounds.y2); // crashes on Natron if kSupportsTiles=0 & kSupportsMultiResolution=1
        }
        if (!kSupportsMultiResolution) {
            // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
            //   Multiple resolution images mean...
            //    input and output images can be of any size
            //    input and output images can be offset from the origin
            assert(srcRod.x1 == 0);
            assert(srcRod.y1 == 0);
            assert(srcRod.x1 == dstRod.x1);
            assert(srcRod.x2 == dstRod.x2);
            assert(srcRod.y1 == dstRod.y1);
            assert(srcRod.y2 == dstRod.y2); // crashes on Natron if kSupportsMultiResolution=0
        }
    }

    double x, y;
    position_->getValueAtTime(args.time, x, y);
    std::string text;
    text_->getValueAtTime(args.time, text);
    int fontSize;
    fontSize_->getValueAtTime(args.time, fontSize);
    int fontName;
    fontName_->getValueAtTime(args.time, fontName);
    double r, g, b, a;
    textColor_->getValueAtTime(args.time, r, g, b, a);
    float textColor[4];
    textColor[0] = (float)r;
    textColor[1] = (float)g;
    textColor[2] = (float)b;
    textColor[3] = (float)a;

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

    // Proc
    magickImage.flip();
    magickImage.font(fontFile);
    //magickImage.fillColor(Magick::Color(r,g,b,MaxRGB));
    magickImage.fillColor("red");
    magickImage.fontPointsize(fontSize);
    magickImage.annotate(text,Magick::CenterGravity);

    /*magickImage.draw(Magick::DrawableFont(fontFile));
    magickImage.draw(Magick::DrawableStrokeColor("black"));
    magickImage.draw(Magick::DrawableFillColor(Magick::Color(0, 0, 0, MaxRGB)));
    magickImage.draw(Magick::DrawableTextUnderColor("white"));
    magickImage.draw(Magick::DrawableText(x,y,text));*/


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

/*
    // allocate temporary image
    int pixelBytes = pixelComponentCount * getComponentBytes(bitDepth);
    int tmpRowBytes = (args.renderWindow.x2-args.renderWindow.x1) * pixelBytes;
    size_t memSize = (args.renderWindow.y2-args.renderWindow.y1) * tmpRowBytes;
    OFX::ImageMemory mem(memSize,this);
    float *tmpPixelData = (float*)mem.lock();


    const bool flipit = true;
    OIIO::ImageSpec tmpSpec = imageSpecFromOFXImage(srcImg.get() ? srcRod : args.renderWindow, args.renderWindow, pixelComponents, bitDepth);
    assert(tmpSpec.width == args.renderWindow.x2 - args.renderWindow.x1);
    assert(tmpSpec.width == args.renderWindow.x2 - args.renderWindow.x1);
    assert(tmpSpec.height == args.renderWindow.y2 - args.renderWindow.y1);
    OIIO::ROI srcRoi(tmpSpec.x, tmpSpec.x + tmpSpec.width, tmpSpec.y, tmpSpec.y+tmpSpec.height);
    int ytext = int(y*args.renderScale.y);
    if (flipit) {
        // from the OIIO 1.5 release notes:
        // Fixes, minor enhancements, and performance improvements:
        // * ImageBufAlgo:
        //   * flip(), flop(), flipflop() have been rewritten to work more
        //     sensibly for cropped images. In particular, the transformation now
        //     happens with respect to the display (full) window, rather than
        //     simply flipping or flopping within the data window. (1.5.2)
#if OIIO_VERSION >= 10502
        // the transformation happens with respect to the display (full) window
        tmpSpec.y = ((tmpSpec.full_y+tmpSpec.full_height-1) - tmpSpec.y) - (tmpSpec.height-1);
        tmpSpec.full_y = 0;
        ytext = (tmpSpec.full_y+tmpSpec.full_height-1) - ytext;
#else
        // only the data window is flipped
        ytext = tmpSpec.y + ((tmpSpec.y+tmpSpec.height-1) - ytext);
#endif
    }
    assert(tmpSpec.width == args.renderWindow.x2 - args.renderWindow.x1);
    assert(tmpSpec.height == args.renderWindow.y2 - args.renderWindow.y1);
    OIIO::ImageBuf tmpBuf("tmp", tmpSpec, tmpPixelData);

    // do we have to flip the image?
    if (!srcImg.get()) {
        OIIO::ImageBufAlgo::zero(tmpBuf);
    } else {
        if (flipit) {
            bool ok = OIIO::ImageBufAlgo::flip(tmpBuf, *srcBuf, srcRoi);
            if (!ok) {
                setPersistentMessage(OFX::Message::eMessageError, "", tmpBuf.geterror().c_str());
                throwSuiteStatusException(kOfxStatFailed);
            }
        } else {
            // copy the renderWindow from src to a temp buffer
            tmpBuf.copy_pixels(*srcBuf);
        }
    }

    // render text in the temp buffer
    {
        bool ok = OIIO::ImageBufAlgo::render_text(tmpBuf, int(x*args.renderScale.x), ytext, text, int(fontSize*args.renderScale.y), fontFile, textColor);
        if (!ok) {
            setPersistentMessage(OFX::Message::eMessageError, "", tmpBuf.geterror().c_str());
            //throwSuiteStatusException(kOfxStatFailed);
        }
    }
    OIIO::ImageSpec dstSpec = imageSpecFromOFXImage(dstRod, dstBounds, pixelComponents, bitDepth);
    OIIO::ImageBuf dstBuf("dst", dstSpec, dstImg->getPixelData());
    
    OIIO::ROI tmpRoi(tmpSpec.x, tmpSpec.x + tmpSpec.width, tmpSpec.y, tmpSpec.y+tmpSpec.height);
    // do we have to flip the image?
    if (flipit) {
        bool ok = OIIO::ImageBufAlgo::flip(dstBuf, tmpBuf, tmpRoi);
        if (!ok) {
            setPersistentMessage(OFX::Message::eMessageError, "", tmpBuf.geterror().c_str());
            throwSuiteStatusException(kOfxStatFailed);
        }
    } else {
        // copy the temp buffer to dstImg
        //dstBuf.copy_pixels(tmpBuf); // erases everything out of tmpBuf!
        bool ok = OIIO::ImageBufAlgo::paste(dstBuf, args.renderWindow.x1, srcRod.y2 - args.renderWindow.y2, 0, 0, tmpBuf, tmpRoi);
        if (!ok) {
            setPersistentMessage(OFX::Message::eMessageError, "", tmpBuf.geterror().c_str());
            throwSuiteStatusException(kOfxStatFailed);
        }
    }

    // TODO: answer questions:
    // - can we support tiling?
    // - can we support multiresolution by just scaling the coordinates and the font size?
*/
}

bool
MagickTextPlugin::isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &/*identityTime*/)
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

    return false;
}

void
MagickTextPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &/*paramName*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    clearPersistentMessage();
}

bool
MagickTextPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
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
    desc.addSupportedBitDepth(eBitDepthUByte);
    desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthHalf);
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
    
    // Source clip only in the filter context
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
        RGBAParamDescriptor* param = desc.defineRGBAParam(kParamTextColor);
        param->setLabel(kParamTextColorLabel);
        param->setHint(kParamTextColorHint);
        param->setDefault(1., 1., 1., 1.);
        param->setAnimates(true);
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
