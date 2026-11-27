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

#include <iostream>
#include <vector>

#include <librsvg/rsvg.h>

#ifdef LEGACY
#include "librsvg/rsvg-cairo.h"
#endif

#include <librevenge/librevenge.h>
#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>
#include <libcdr/libcdr.h>

#include "GenericReader.h"
#include "GenericOCIO.h"
#include "PackedImage.h"
#include "ofxsImageEffect.h"
#include "ReadHandles.h"

#define kPluginName "ReadCDR"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadCDR"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0
#define kPluginEvaluation 50

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch (90 is default)"
#define kParamDpiDefault 90

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsXY false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar false

using namespace OFX;
using namespace OFX::IO;

#ifdef OFX_IO_USING_OCIO
namespace OCIO = OCIO_NAMESPACE;
#endif

OFXS_NAMESPACE_ANONYMOUS_ENTER

class ReadCDRPlugin : public GenericReaderPlugin
{
public:
    ReadCDRPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadCDRPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, const OfxRectI& renderWindow, const OfxPointD& renderScale, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, int view, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error,int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual bool guessParamsFromFilename(const std::string& filename, std::string *colorspace, OFX::PreMultiplicationEnum *filePremult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    std::string readSVG(const std::string& filename);
    Arena::GObjectPtr<RsvgHandle> openSVG(const std::string& svg);
    OFX::IntParam *_dpi;
};

ReadCDRPlugin::ReadCDRPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles, kIsMultiPlanar)
,_dpi(NULL)
{
    _dpi = fetchIntParam(kParamDpi);
    assert(_dpi);
}

ReadCDRPlugin::~ReadCDRPlugin()
{
}

// The first page of the CorelDRAW document filename, as an SVG document. Throws, with a
// persistent message, if it can't be read.
std::string
ReadCDRPlugin::readSVG(const std::string& filename)
{
    librevenge::RVNGFileStream input(filename.c_str());
    if (!libcdr::CDRDocument::isSupported(&input)) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unsupported file format");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    librevenge::RVNGStringVector output;
    librevenge::RVNGSVGDrawingGenerator generator(output, "svg");
    if (!libcdr::CDRDocument::parse(&input, &generator)) {
        setPersistentMessage(OFX::Message::eMessageError, "", "SVG generation failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    if (output.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No SVG document generated");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // The generator makes one SVG document per page. Only the first page is read: concatenating
    // them all under one XML declaration made a document with several root elements, which is
    // not XML, so files with more than one page always failed with "Failed to read SVG" (#133).
    std::string svg = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
    svg += output[0].cstr();
    return svg;
}

// An rsvg handle on the SVG document svg. Throws, with a persistent message, if it can't be read.
Arena::GObjectPtr<RsvgHandle>
ReadCDRPlugin::openSVG(const std::string& svg)
{
    GError* e = NULL;
    Arena::GObjectPtr<RsvgHandle> handle(rsvg_handle_new_from_data(reinterpret_cast<const guint8*>(svg.data()), svg.size(), &e));
    const Arena::GErrorPtr error(e);
    if (error || !handle) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    return handle;
}

void
ReadCDRPlugin::decode(const std::string& filename,
                      OfxTime time,
                      int /*view*/,
                      const OfxRectI& renderWindow,
                      const OfxPointD& renderScale,
                      float *pixelData,
                      const OfxRectI& bounds,
                      OFX::PixelComponentEnum /*pixelComponents*/,
                      int pixelComponentCount,
                      int rowBytes)
{
    assert(renderScale.x == 1. && renderScale.y == 1.);
    unused(renderScale);
    if (pixelComponentCount != 4) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Wrong pixel components");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No file");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    const std::string svg = readSVG(filename);

    // the handles are released on every path, including the throws below (#132)
    RsvgDimensionData dimension;
    cairo_status_t status;
    double imageWidth, imageHeight, scaleWidth, scaleHeight;
    int dpi, width, height;
    _dpi->getValueAtTime(time, dpi);

    const Arena::GObjectPtr<RsvgHandle> handle = openSVG(svg);
    rsvg_handle_set_dpi_x_y(handle.get(), dpi, dpi);

    rsvg_handle_get_dimensions(handle.get(), &dimension);

    imageWidth = dimension.width;
    imageHeight = dimension.height;

    if (dpi != kParamDpiDefault) {
        width = Arena::toPixels(imageWidth * dpi / kParamDpiDefault);
        height = Arena::toPixels(imageHeight * dpi / kParamDpiDefault);
    }
    else {
        width = Arena::toPixels(imageWidth);
        height = Arena::toPixels(imageHeight);
    }

    // The whole drawing is rendered, at (0,0,width,height) as in getFrameBounds(), and only the
    // part of renderWindow that it covers is copied: a host may ask for a render window smaller
    // than the image, even without tiles (#105).
    OfxRectI image;
    image.x1 = 0;
    image.y1 = 0;
    image.x2 = width;
    image.y2 = height;

    // An empty canvas (a zero width or height) gave 0 / 0 = NaN scales below, which only
    // cairo's check of the matrix turned into the "Render failed" error: fail here instead.
    if (imageWidth <= 0. || imageHeight <= 0. || width <= 0 || height <= 0) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    const Arena::CairoSurfacePtr surfacePtr(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height));
    const Arena::CairoPtr crPtr(cairo_create(surfacePtr.get()));
    cairo_surface_t* const surface = surfacePtr.get();
    cairo_t* const cr = crPtr.get();

    scaleWidth = width / imageWidth;
    scaleHeight = height / imageHeight;

    cairo_scale(cr, scaleWidth, scaleHeight);

    rsvg_handle_render_cairo(handle.get(), cr);
    status = cairo_status(cr);

    if (status) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // copy the part of renderWindow inside the image, and clear the rest (see ReadSVG.cpp)
    OfxRectI copied;
    clipToFile(renderWindow, image, pixelData, bounds, pixelComponentCount, OFX::eBitDepthFloat, rowBytes, &copied);
    Arena::copyCairoSurface(surface, copied, pixelData, bounds, rowBytes);
}

bool ReadCDRPlugin::getFrameBounds(const std::string& filename,
                              OfxTime time,
                                   int /*view*/,
                              OfxRectI *bounds,
                                   OfxRectI* format,
                              double *par,
                              std::string* errorMsg,int *tile_width, int *tile_height)
{
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    const std::string svg = readSVG(filename);

    int dpi;
    _dpi->getValueAtTime(time, dpi);

    RsvgDimensionData dimension;
    double imageWidth, imageHeight;
    int width, height;

    const Arena::GObjectPtr<RsvgHandle> handle = openSVG(svg);
    rsvg_handle_set_dpi_x_y(handle.get(), dpi, dpi);

    rsvg_handle_get_dimensions(handle.get(), &dimension);

    imageWidth = dimension.width;
    imageHeight = dimension.height;

    if (dpi != kParamDpiDefault) {
        width = Arena::toPixels(imageWidth * dpi / kParamDpiDefault);
        height = Arena::toPixels(imageHeight * dpi / kParamDpiDefault);
    } else {
        width = Arena::toPixels(imageWidth);
        height = Arena::toPixels(imageHeight);
    }

    if (width <= 0 || height <= 0) {
        // an empty drawing: report it, instead of returning true with the bounds unset, from
        // which GenericReader made a garbage RoD (#129)
        if (errorMsg) {
            *errorMsg = "Empty or too large CorelDRAW page";
        }
        return false;
    }
    bounds->x1 = 0;
    bounds->x2 = width;
    bounds->y1 = 0;
    bounds->y2 = height;
    *format = *bounds;
    *par = 1.0;

    *tile_width = *tile_height = 0;
    return true;
}

bool ReadCDRPlugin::guessParamsFromFilename(const std::string& /*newFile*/,
                                       std::string *colorspace,
                                       OFX::PreMultiplicationEnum *filePremult,
                                       OFX::PixelComponentEnum *components,
                                       int *componentCount)
{
    assert(colorspace && filePremult && components && componentCount);
# ifdef OFX_IO_USING_OCIO
    *colorspace = "sRGB";
# endif
    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st != kOfxStatOK || filename.empty() ) {
        return false;
    }

    const std::string svg = readSVG(filename);

    openSVG(svg); // throws if it can't be read

    *components = OFX::ePixelComponentRGBA;
    *filePremult = OFX::eImagePreMultiplied;

    return true;
}


using namespace OFX;

mDeclareReaderPluginFactory(ReadCDRPluginFactory, {}, false);

void
ReadCDRPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("cdr");
}

/** @brief The basic describe function, passed a plugin descriptor */
void ReadCDRPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    desc.setPluginDescription("Read CorelDRAW(R) document format.\n\nThis plugin is not manufactured, approved, or supported by Corel Corporation or Corel Corporation Limited.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadCDRPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles, false);
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamDpi);
        param->setLabel(kParamDpiLabel);
        param->setHint(kParamDpiHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamDpiDefault);
        param->setAnimates(false);
        page->addChild(*param);
        // the format, set in getClipPreferences(), depends on the DPI
        desc.addClipPreferencesSlaveParam(*param);
    }

    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "scene_linear");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadCDRPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadCDRPlugin* ret =  new ReadCDRPlugin(handle, _extensions);
    ret->restoreStateFromParams();
    return ret;
}

static ReadCDRPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
