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

#include <poppler.h>
#include <cairo.h>

#include <iostream>

#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsImageEffect.h"

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadPDF"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadPDF"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0
#define kPluginEvaluation 50
#define kPluginDPI 72

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch (150 is default)"
#define kParamDpiDefault 150

#define kParamPage "page"
#define kParamPageLabel "Page"
#define kParamPageHint "Document page"

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

static bool gHostIsNatron = false;

class ReadPDFPlugin : public GenericReaderPlugin
{
public:
    ReadPDFPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadPDFPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error,int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void restoreState(const std::string& filename) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    void getPageNum(std::string filename);
    OFX::IntParam *_dpi;
    OFX::ChoiceParam *_page;
};

ReadPDFPlugin::ReadPDFPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
,_dpi(0)
{
    _dpi = fetchIntParam(kParamDpi);
    _page = fetchChoiceParam(kParamPage);
    assert(_dpi && _page);
}

ReadPDFPlugin::~ReadPDFPlugin()
{
}

void
ReadPDFPlugin::decode(const std::string& filename,
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
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    GError *error = NULL;
    PopplerDocument *document = NULL;
    PopplerPage *page = NULL;
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_status_t status;
    double imageWidth, imageHeight, scaleWidth, scaleHeight;
    int dpi, width, height, renderWidth, renderHeight, pageNum;
    _dpi->getValueAtTime(time, dpi);
    _page->getValueAtTime(time, pageNum);

    gchar *absolute, *uri;
    absolute = g_strdup(filename.c_str());
    uri = g_filename_to_uri(absolute, NULL, &error);
    free(absolute);

    document = poppler_document_new_from_file(uri, NULL, &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    page = poppler_document_get_page(document, pageNum);
    if (page == NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read page");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    poppler_page_get_size(page, &imageWidth, &imageHeight);

    renderWidth= renderWindow.x2 - renderWindow.x1;
    renderHeight= renderWindow.y2 - renderWindow.y1;

    width = dpi * imageWidth / kPluginDPI;
    height = dpi * imageHeight / kPluginDPI;

    if (width != renderWidth || height != renderHeight) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Image don't match RenderWindow");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr=cairo_create(surface);

    scaleWidth = dpi/kPluginDPI;
    scaleHeight = dpi/kPluginDPI;

    cairo_scale(cr, scaleWidth, scaleHeight);

    poppler_page_render(page, cr);
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

    g_object_unref(page);
    g_object_unref(document);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cdata = NULL;
    error = NULL;
    delete[] pixels;
}

bool ReadPDFPlugin::getFrameBounds(const std::string& filename,
                              OfxTime time,
                              OfxRectI *bounds,
                              double *par,
                              std::string* /*error*/,int *tile_width, int *tile_height)
{
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int dpi, pageNum;
    _dpi->getValueAtTime(time, dpi);
    _page->getValueAtTime(time, pageNum);

    GError *error = NULL;
    PopplerDocument *document = NULL;
    PopplerPage *page = NULL;
    double imageWidth, imageHeight;
    int width = 0;
    int height = 0;

    gchar *absolute, *uri;
    absolute = g_strdup(filename.c_str());
    uri = g_filename_to_uri(absolute, NULL, &error);
    free(absolute);

    document = poppler_document_new_from_file(uri, NULL, &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    page = poppler_document_get_page(document, pageNum);
    poppler_page_get_size(page, &imageWidth, &imageHeight);
    if (page !=NULL) {
        width = dpi * imageWidth / kPluginDPI;
        height = dpi * imageHeight / kPluginDPI;
    }

    g_object_unref(page);
    g_object_unref(document);
    error = NULL;

    if (width > 0 && height > 0) {
        bounds->x1 = 0;
        bounds->x2 = width;
        bounds->y1 = 0;
        bounds->y2 = height;
        *par = 1.0;
    }

    *tile_width = *tile_height = 0;
    return true;
}

void ReadPDFPlugin::restoreState(const std::string& filename)
{
    getPageNum(filename);
}

void ReadPDFPlugin::getPageNum(std::string filename)
{
    if (!filename.empty() && gHostIsNatron) {
        _page->resetOptions();

        GError *error = NULL;
        PopplerDocument *document = NULL;
        int pages = 0;

        gchar *absolute, *uri;
        absolute = g_strdup(filename.c_str());
        uri = g_filename_to_uri(absolute, NULL, &error);
        free(absolute);

        document = poppler_document_new_from_file(uri, NULL, &error);

        if (error != NULL) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
            OFX::throwSuiteStatusException(kOfxStatErrFormat);
        }

        pages = poppler_document_get_n_pages(document)-1;
        if (pages<0)
            pages=0;

        for (int i = 0; i < pages; i++) {
            std::ostringstream pageName;
            pageName << "Page " << i;
            _page->appendOption(pageName.str());
        }

        g_object_unref(document);
        error = NULL;
    }
}

void ReadPDFPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    assert(premult && components);

    if (newFile.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    GError *error = NULL;
    PopplerDocument *document = NULL;

    gchar *absolute, *uri;
    absolute = g_strdup(newFile.c_str());
    uri = g_filename_to_uri(absolute, NULL, &error);
    free(absolute);

    document = poppler_document_new_from_file(uri, NULL, &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    getPageNum(newFile);

    g_object_unref(document);
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

mDeclareReaderPluginFactory(ReadPDFPluginFactory, {}, false);

void
ReadPDFPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("pdf");
}


/** @brief The basic describe function, passed a plugin descriptor */
void ReadPDFPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, false);
    desc.setLabel(kPluginName);
    desc.setPluginDescription("Read PDF");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadPDFPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false);
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamDpi);
        param->setLabel(kParamDpiLabel);
        param->setHint(kParamDpiHint);
        param->setRange(1, 10000);
        param->setDisplayRange(1, 500);
        param->setDefault(kParamDpiDefault);
        param->setAnimates(false);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamPage);
        param->setLabel(kParamPageLabel);
        param->setHint(kParamPageHint);
        param->appendOption("Default");
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }

    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadPDFPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadPDFPlugin* ret =  new ReadPDFPlugin(handle, _extensions);
    ret->restoreStateFromParameters();
    return ret;
}

static ReadPDFPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
