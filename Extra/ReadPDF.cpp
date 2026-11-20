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

#include <poppler.h>
#include <GlobalParams.h>
#include <cairo.h>
#include <string>
#include <vector>
#include <sstream>
#include <mutex>

#include <iostream>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __linux__
#include <unistd.h>
#include <libgen.h>
#include <cstring>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <libgen.h>
#include <cstring>
#elif _WIN32
#include <windows.h>
#include <Shlwapi.h>
#endif

#if defined(POPPLER_MAJOR_VERSION) && defined(POPPLER_MINOR_VERSION)
#if POPPLER_MAJOR_VERSION == 0 && POPPLER_MINOR_VERSION < 83
#define LEGACY_POPPLER
#endif
#endif

#include <ofxNatron.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "PackedImage.h"
#include "ofxsMacros.h"
#include "ofxsMultiPlane.h"
#include "ofxsImageEffect.h"

#define kPluginName "ReadPDF"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadPDF"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 4
#define kPluginEvaluation 50
#define kPluginDPI 72.0

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch (150 is default)"
#define kParamDpiDefault 150.0

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsXY false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar true

using namespace OFX;
using namespace OFX::IO;

#ifdef OFX_IO_USING_OCIO
namespace OCIO = OCIO_NAMESPACE;
#endif

OFXS_NAMESPACE_ANONYMOUS_ENTER

static bool gHostIsNatron = false;

// Open a PDF file: NULL if it can't be read. The URI, the GError and the copy of the
// filename are freed here, on every path (they leaked, and the copy was free()d although
// g_strdup() memory must be g_free()d).
static PopplerDocument*
openDocument(const std::string& filename)
{
    GError *error = NULL;
    gchar *uri = g_filename_to_uri(filename.c_str(), NULL, &error);
    if (uri == NULL) {
        if (error) {
            g_error_free(error);
        }
        return NULL;
    }
    PopplerDocument *document = poppler_document_new_from_file(uri, NULL, &error);
    g_free(uri);
    if (error) {
        g_error_free(error);
        if (document) {
            g_object_unref(document);
        }
        return NULL;
    }
    return document;
}

// Unrefs the poppler objects and destroys the cairo objects it holds when it goes out of
// scope, so that the error paths, which throw, release them too.
struct PDFResources
{
    PopplerDocument *document;
    PopplerPage *page;
    PopplerPage *fpage;
    cairo_surface_t *surface;
    cairo_t *cr;

    PDFResources() : document(NULL), page(NULL), fpage(NULL), surface(NULL), cr(NULL) {}
    ~PDFResources()
    {
        if (cr) { cairo_destroy(cr); }
        if (surface) { cairo_surface_destroy(surface); }
        if (page) { g_object_unref(page); }
        if (fpage) { g_object_unref(fpage); }
        if (document) { g_object_unref(document); }
    }
private:
    PDFResources(const PDFResources&);
    PDFResources& operator=(const PDFResources&);
};

// the page names ("0", "1", ...) of a document, empty if it is NULL
static std::vector<std::string>
pageNames(PopplerDocument* document)
{
    std::vector<std::string> names;
    const int pages = document ? poppler_document_get_n_pages(document) : 0;
    for (int i = 0; i < pages; i++) {
        std::ostringstream pageName;
        pageName << i;
        names.push_back(pageName.str());
    }
    return names;
}

class ReadPDFPlugin : public GenericReaderPlugin
{
public:
    ReadPDFPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadPDFPlugin();
    virtual void restoreStateFromParams() OVERRIDE FINAL;
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, const OfxRectI& renderWindow, const OfxPointD& renderScale, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL
    {
        std::string rawComps;
        switch (pixelComponents) {
            case OFX::ePixelComponentAlpha:
                rawComps = kOfxImageComponentAlpha;
                break;
            case OFX::ePixelComponentRGB:
                rawComps = kOfxImageComponentRGB;
                break;
            case OFX::ePixelComponentRGBA:
                rawComps = kOfxImageComponentRGBA;
                break;
            default:
                OFX::throwSuiteStatusException(kOfxStatFailed);
                return;
        }
        decodePlane(filename, time, view, renderWindow, renderScale, pixelData, bounds, pixelComponents, pixelComponents, pixelComponentCount, rawComps, rowBytes);
    }
    virtual void decodePlane(const std::string& filename, OfxTime time, int view, const OfxRectI& renderWindow, const OfxPointD& renderScale, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, OFX::PixelComponentEnum remappedComponents, int pixelComponentCount, const std::string& rawComponents, int rowBytes) OVERRIDE FINAL;
    virtual OfxStatus getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, int view, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error,int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual bool guessParamsFromFilename(const std::string& filename, std::string *colorspace, OFX::PreMultiplicationEnum *filePremult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    virtual void changedFilename(const OFX::InstanceChangedArgs &args) OVERRIDE FINAL;
    std::string getResourcesPath();
    std::vector<std::string> imageLayers;
    OFX::DoubleParam *_dpi;
};

ReadPDFPlugin::ReadPDFPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles,
#ifdef OFX_EXTENSIONS_NUKE
(OFX::getImageEffectHostDescription() && OFX::getImageEffectHostDescription()->isMultiPlanar) ? kIsMultiPlanar : false
#else
false
#endif
)
,_dpi(NULL)
{
    _dpi = fetchDoubleParam(kParamDpi);
    assert(_dpi);

    // When we deploy the plugin we need access to the poppler data folder,
    // we asume that the path is OFX_HOST_BINARY/../Resources/poppler.
    // If not found, whatever poppler has as default will be used (may or may not work).
    // globalParams is process-wide: set it only once. Replacing it in every
    // instance constructor destroyed the previous object while other
    // instances could be rendering with it on other threads.
    static std::once_flag popplerDataOnce;
    std::call_once(popplerDataOnce, [this]() {
        // popplerDataOnce lives in this binary, but globalParams lives in libpoppler: if the host
        // unloads the binary and loads it again (HostSupport does after its scan, openfx #174)
        // while libpoppler stays loaded, the flag is fresh but globalParams may already be set,
        // by the previous load or by poppler-glib. Keep it: replacing it is the bug above.
        if (globalParams) {
            return;
        }
        std::string popplerData = getResourcesPath();
        if (!popplerData.empty()) {
            struct stat sb;
            if (stat(popplerData.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
#ifdef LEGACY_POPPLER
                globalParams = new GlobalParams(popplerData.c_str());
#else
                globalParams.reset(new GlobalParams(popplerData.c_str()));
#endif
            }
        }
    });
}

ReadPDFPlugin::~ReadPDFPlugin()
{
}

void ReadPDFPlugin::restoreStateFromParams()
{
    GenericReaderPlugin::restoreStateFromParams();

    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    // && (was ||): with no file set, this tried to open "" and reported an error
    if ( st == kOfxStatOK && !filename.empty() ) {
        PopplerDocument *document = openDocument(filename);
        if (document == NULL) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        }
        imageLayers = pageNames(document);
        if (document) {
            g_object_unref(document);
        }
    }
}

std::string
ReadPDFPlugin::getResourcesPath() {
    std::string result;
    char path[PATH_MAX];

#ifdef __linux__
    char tmp[PATH_MAX];
    if (readlink("/proc/self/exe", tmp, sizeof(tmp)) != -1) {
        strcpy(path,dirname(tmp));
    }
#elif __APPLE__
    char tmp[PATH_MAX];
    uint32_t size = sizeof(tmp);
    _NSGetExecutablePath(tmp, &size);
    strcpy(path,dirname(tmp));
#elif _WIN32
    HMODULE hModule = GetModuleHandle(NULL);
    if (hModule != NULL) {
        GetModuleFileName(hModule,path, (sizeof(path)));
        PathRemoveFileSpec(path);
    }
#endif

    result = path;
    if (!result.empty()) {
#ifdef _WIN32
        result += "\\..\\Resources\\poppler";
#else
        result += "/../Resources/poppler";
#endif
    }

    return result;
}

OfxStatus
ReadPDFPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    assert(isMultiPlanar());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (imageLayers.size()>0 && gHostIsNatron) {
        for (int i = 0; i < (int)imageLayers.size(); i++) {
            std::string layerName;
            {
                std::ostringstream ss;
                ss << imageLayers[i];
                layerName = ss.str();
            }
            const char* components[4] = {"R","G","B", "A"};
            OFX::MultiPlane::ImagePlaneDesc plane(layerName, layerName, "", components, 4);
            clipComponents.addClipPlane(*_outputClip, OFX::MultiPlane::ImagePlaneDesc::mapPlaneToOFXPlaneString(plane));
        }

        // Also add the color plane
        clipComponents.addClipPlane(*_outputClip, OFX::MultiPlane::ImagePlaneDesc::mapPlaneToOFXPlaneString(OFX::MultiPlane::ImagePlaneDesc::getRGBAComponents()));
    }
    return kOfxStatOK;
}

void
ReadPDFPlugin::decodePlane(const std::string& filename, OfxTime time, int /*view*/, const OfxRectI& renderWindow, const OfxPointD& renderScale, float *pixelData, const OfxRectI& bounds,
                                 OFX::PixelComponentEnum /*pixelComponents*/, OFX::PixelComponentEnum /*remappedComponents*/, int pixelComponentCount, const std::string& rawComponents, int rowBytes)
{
    assert(renderScale.x == 1. && renderScale.y == 1.);
    unused(renderScale);
    if (pixelComponentCount != 4) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Wrong pixel components");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    PDFResources r; // released on every path, including the throws below
    cairo_status_t status;
    double dpi, imageWidth, imageHeight, scaleWidth, scaleHeight;
    int width, height;
    _dpi->getValueAtTime(time, dpi);

    int layer = 0;
    if (gHostIsNatron) {
        OFX::MultiPlane::ImagePlaneDesc plane, pairedPlane;
        OFX::MultiPlane::ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(rawComponents, &plane, &pairedPlane);

        if (!plane.isColorPlane()) {
            layer = atoi(plane.getPlaneLabel().c_str());
        }
    }

    r.document = openDocument(filename);
    if (r.document == NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // poppler reports a page index out of range as a GLib critical: check it first
    const int nPages = poppler_document_get_n_pages(r.document);
    if (layer >= 0 && layer < nPages) {
        r.page = poppler_document_get_page(r.document, layer);
    }
    if (nPages > 0) {
        r.fpage = poppler_document_get_page(r.document, 0);
    }
    if (r.page == NULL || r.fpage == NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read page");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    poppler_page_get_size(r.fpage, &imageWidth, &imageHeight);

    width = dpi * imageWidth / kPluginDPI;
    height = dpi * imageHeight / kPluginDPI;

    // The whole page is rendered, at (0,0,width,height) as in getFrameBounds(), and only the part
    // of renderWindow that it covers is copied: a host may ask for a render window smaller than
    // the image, even without tiles (#105). This used to fail unless renderWindow was the page.
    // (an empty page used to fail as "Image don't match RenderWindow")
    if (width <= 0 || height <= 0) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    OfxRectI image;
    image.x1 = 0;
    image.y1 = 0;
    image.x2 = width;
    image.y2 = height;

    r.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    r.cr = cairo_create(r.surface);
    cairo_t *cr = r.cr;

    scaleWidth = dpi/kPluginDPI;
    scaleHeight = dpi/kPluginDPI;

    cairo_scale(cr, scaleWidth, scaleHeight);

    poppler_page_render(r.page, cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    status = cairo_status(cr);

    if (status) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    cairo_surface_flush(r.surface);

    unsigned char* cdata = cairo_image_surface_get_data(r.surface);
    std::vector<unsigned char> pixels(width * height * pixelComponentCount);
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            for (int k = 0; k < pixelComponentCount; ++k)
                pixels[(i + j * width) * pixelComponentCount + k] = cdata[(i + (height - 1 - j) * width) * pixelComponentCount + k];
        }
    }

    // pixels is packed, bottom-up, and covers image. Copy the part of renderWindow inside the
    // image (clamped, so that a window larger than the image or outside of it doesn't read out of
    // pixels), and clear the rest of the window. pixelData has the given bounds and rowBytes,
    // which may differ from renderWindow, be padded, or be negative.
    // GenericReader's clipToFile() does both (copied is empty if the window is not in the
    // image), and tells render() that only copied comes from the file: call it once per decode(),
    // with decode()'s renderWindow (see GenericReader.h)
    OfxRectI copied;
    clipToFile(renderWindow, image, pixelData, bounds, pixelComponentCount, OFX::eBitDepthFloat, rowBytes, &copied);
    for (int y = copied.y1; y < copied.y2; y++) {
        size_t offset = ((size_t)y * width + copied.x1) * pixelComponentCount;
        float* dst = PackedImage::pixelAddress(pixelData, bounds, rowBytes, pixelComponentCount, copied.x1, y);
        for (int x = copied.x1; x < copied.x2; x++) {
            dst[0] = pixels[offset + 2] * (1.f / 255);
            dst[1] = pixels[offset + 1] * (1.f / 255);
            dst[2] = pixels[offset + 0] * (1.f / 255);
            dst[3] = pixels[offset + 3] * (1.f / 255);
            dst += pixelComponentCount;
            offset += pixelComponentCount;
        }
    }

}

bool ReadPDFPlugin::getFrameBounds(const std::string& filename,
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

    double dpi;
    _dpi->getValueAtTime(time, dpi);

    PopplerPage *page = NULL;
    double imageWidth, imageHeight;
    int width = 0;
    int height = 0;

    PopplerDocument *document = openDocument(filename);
    if (document == NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // a document without pages has no page 0: check it before reading its size (and before
    // asking for it, which poppler reports as a critical error)
    if (poppler_document_get_n_pages(document) > 0) {
        page = poppler_document_get_page(document, 0);
    }
    if (page != NULL) {
        poppler_page_get_size(page, &imageWidth, &imageHeight);
        width = dpi * imageWidth / kPluginDPI;
        height = dpi * imageHeight / kPluginDPI;
        g_object_unref(page);
    }

    g_object_unref(document);

    if (width <= 0 || height <= 0) {
        // bounds and format were not set: report failure instead of returning garbage
        // bounds to GenericReader
        if (errorMsg) {
            *errorMsg = page == NULL ? "Failed to read page" : "Empty page";
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

bool ReadPDFPlugin::guessParamsFromFilename(const std::string& /*newFile*/,
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

    PopplerDocument *document = openDocument(filename);
    if (document == NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
    }
    imageLayers = pageNames(document);
    if (document) {
        g_object_unref(document);
    }

    *components = OFX::ePixelComponentRGBA;
    *filePremult = OFX::eImageUnPreMultiplied;

    return true;
}

void ReadPDFPlugin::changedFilename(const OFX::InstanceChangedArgs &args)
{
    GenericReaderPlugin::changedFilename(args);

    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st != kOfxStatOK || filename.empty() ) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    PopplerDocument *document = openDocument(filename);
    if (document == NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
    }
    imageLayers = pageNames(document);
    if (document) {
        g_object_unref(document);
    }
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
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, kIsMultiPlanar);
    desc.setLabel(kPluginName);
    desc.setPluginDescription("Read PDF documents using poppler.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadPDFPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB,kSupportsXY, kSupportsAlpha, kSupportsTiles, false);
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamDpi);
        param->setLabel(kParamDpiLabel);
        param->setHint(kParamDpiHint);
        param->setRange(1, 10000);
        param->setDisplayRange(1, 500);
        param->setDefault(kParamDpiDefault);
        param->setAnimates(false);
        page->addChild(*param);
    }

    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "scene_linear");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadPDFPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadPDFPlugin* ret =  new ReadPDFPlugin(handle, _extensions);
    ret->restoreStateFromParams();
    return ret;
}

static ReadPDFPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
