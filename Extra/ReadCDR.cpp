/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
 * Copyright (C) 2015, 2016 FxArena DA
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
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
#include "ofxsImageEffect.h"

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

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

class ReadCDRPlugin : public GenericReaderPlugin
{
public:
    ReadCDRPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadCDRPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error,int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    OFX::IntParam *_dpi;
};

ReadCDRPlugin::ReadCDRPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
,_dpi(0)
{
    _dpi = fetchIntParam(kParamDpi);
    assert(_dpi);
}

ReadCDRPlugin::~ReadCDRPlugin()
{
}

void
ReadCDRPlugin::decode(const std::string& filename,
                      OfxTime time,
                      int /*view*/,
                      bool /*isPlayback*/,
                      const OfxRectI& renderWindow,
                      float *pixelData,
                      const OfxRectI& /*bounds*/,
                      OFX::PixelComponentEnum /*pixelComponents*/,
                      int pixelComponentCount,
                      int /*rowBytes*/)
{
    if (pixelComponentCount != 4) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Wrong pixel components");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No file");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

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

    std::ostringstream stream;
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
    for (unsigned k = 0; k<output.size(); ++k)
        stream << output[k].cstr();

    GError *error = NULL;
    RsvgHandle *handle;
    RsvgDimensionData dimension;
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_status_t status;
    double imageWidth, imageHeight, scaleWidth, scaleHeight;
    int dpi, width, height, renderWidth, renderHeight;
    _dpi->getValueAtTime(time, dpi);

    rsvg_set_default_dpi_x_y(dpi, dpi);
    handle=rsvg_handle_new_from_data((guint8 *)stream.str().c_str(), stream.str().size(), &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    rsvg_handle_get_dimensions(handle, &dimension);

    imageWidth = dimension.width;
    imageHeight = dimension.height;
    renderWidth= renderWindow.x2 - renderWindow.x1;
    renderHeight= renderWindow.y2 - renderWindow.y1;

    if (dpi != kParamDpiDefault) {
        width = imageWidth * dpi / kParamDpiDefault;
        height = imageHeight * dpi / kParamDpiDefault;
    }
    else {
        width = imageWidth;
        height = imageHeight;
    }

    if (width != renderWidth || height != renderHeight) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Image don't match RenderWindow");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr=cairo_create(surface);

    scaleWidth = width / imageWidth;
    scaleHeight = height / imageHeight;

    cairo_scale(cr, scaleWidth, scaleHeight);

    rsvg_handle_render_cairo(handle, cr);
    status = cairo_status(cr);

    if (status) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    cairo_surface_flush(surface);

    unsigned char* cdata = cairo_image_surface_get_data(surface);
    unsigned char* pixels = new unsigned char[width * height * pixelComponentCount];
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            for (int k = 0; k < pixelComponentCount; ++k)
                pixels[(i + j * width) * pixelComponentCount + k] = cdata[(i + (height - 1 - j) * width) * pixelComponentCount + k];
        }
    }

    int offset = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixelData[offset] = (float)pixels[offset + 2] / 255.f;
            pixelData[offset + 1] = (float)pixels[offset + 1] / 255.f;
            pixelData[offset + 2] = (float)pixels[offset] / 255.f;
            pixelData[offset + 3] = (float)pixels[offset + 3] / 255.f;
            offset += pixelComponentCount;
        }
    }

    g_object_unref(handle);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cdata = NULL;
    error = NULL;
    delete[] pixels;
}

bool ReadCDRPlugin::getFrameBounds(const std::string& filename,
                              OfxTime time,
                              OfxRectI *bounds,
                              double *par,
                              std::string* /*error*/,int *tile_width, int *tile_height)
{
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

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

    std::ostringstream stream;
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
    for (unsigned k = 0; k<output.size(); ++k)
        stream << output[k].cstr();

    int dpi;
    _dpi->getValueAtTime(time, dpi);

    GError *error = NULL;
    RsvgHandle *handle;
    RsvgDimensionData dimension;
    double imageWidth, imageHeight;
    int width, height;

    rsvg_set_default_dpi_x_y(dpi, dpi);

    handle=rsvg_handle_new_from_data((guint8 *)stream.str().c_str(), stream.str().size(), &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    rsvg_handle_get_dimensions(handle, &dimension);

    imageWidth = dimension.width;
    imageHeight = dimension.height;

    if (dpi != kParamDpiDefault) {
        width = imageWidth * dpi / kParamDpiDefault;
        height = imageHeight * dpi / kParamDpiDefault;
    }
    else {
        width = imageWidth;
        height = imageHeight;
    }

    g_object_unref(handle);
    error = NULL;

    if (width>0 && height>0) {
        bounds->x1 = 0;
        bounds->x2 = width;
        bounds->y1 = 0;
        bounds->y2 = height;
        *par = 1.0;
    }

    *tile_width = *tile_height = 0;
    return true;
}

void ReadCDRPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    assert(premult && components);

    if (newFile.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    librevenge::RVNGFileStream input(newFile.c_str());
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

    std::ostringstream stream;
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
    for (unsigned k = 0; k<output.size(); ++k)
        stream << output[k].cstr();

    GError *error = NULL;
    RsvgHandle *handle;

    handle=rsvg_handle_new_from_data((guint8 *)stream.str().c_str(), stream.str().size(), &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    g_object_unref(handle);
    error = NULL;

    if (setColorSpace) {
# ifdef OFX_IO_USING_OCIO
        _ocio->setInputColorspace("sRGB");
# endif
    }

    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImagePreMultiplied;
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
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsXY kSupportsAlpha, kSupportsTiles, false);
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamDpi);
        param->setLabel(kParamDpiLabel);
        param->setHint(kParamDpiHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 500);
        param->setDefault(kParamDpiDefault);
        param->setAnimates(false);
        page->addChild(*param);
    }

    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadCDRPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadCDRPlugin* ret =  new ReadCDRPlugin(handle, _extensions);
    ret->restoreStateFromParameters();
    return ret;
}

static ReadCDRPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
