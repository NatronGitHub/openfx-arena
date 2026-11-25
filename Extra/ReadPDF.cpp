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
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <mutex>

#include <iostream>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __linux__
#include <climits> // PATH_MAX
#include <unistd.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#elif _WIN32
#include <windows.h>
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
#include "ReadHandles.h"

#define kPluginName "ReadPDF"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadPDF"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 5
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

// Open a PDF file: null if it can't be read. The URI (g_filename_to_uri() memory, which must
// be g_free()d) is freed here.
static Arena::GObjectPtr<PopplerDocument>
openDocument(const std::string& filename)
{
    GError* e = nullptr;
    gchar* uri = g_filename_to_uri(filename.c_str(), nullptr, &e);
    const Arena::GErrorPtr uriError(e);
    if (uri == nullptr) {
        return nullptr;
    }
    e = nullptr;
    Arena::GObjectPtr<PopplerDocument> document(poppler_document_new_from_file(uri, nullptr, &e));
    g_free(uri);
    const Arena::GErrorPtr error(e);
    if (error) {
        return nullptr;
    }
    return document;
}

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
    void setImageLayers(PopplerDocument* document);
    // The page names of the file: set on the main thread, read by getClipComponents(), which the
    // host may call from any thread, so only accessed with the mutex held (#130)
    std::mutex _imageLayersMutex;
    std::vector<std::string> _imageLayers;
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
        const Arena::GObjectPtr<PopplerDocument> document = openDocument(filename);
        if (!document) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        }
        setImageLayers(document.get());
    } else {
        setImageLayers(NULL); // no file: no pages (#210)
    }
}

// The poppler data directory of a deployed plug-in, <directory of the host executable>/../Resources/poppler,
// or "" if the executable path can't be read. (#131: readlink() doesn't NUL-terminate its result,
// the path was used uninitialized when it failed, and the result of _NSGetExecutablePath() was not
// checked, a path longer than the buffer being left unwritten.)
std::string
ReadPDFPlugin::getResourcesPath() {
    std::string exe; // the path of the host executable
#ifdef __linux__
    std::vector<char> buf(PATH_MAX);
    for (;;) {
        const ssize_t n = readlink("/proc/self/exe", buf.data(), buf.size());
        if (n < 0) {
            break;
        }
        if (static_cast<size_t>(n) < buf.size()) { // else it may be truncated: try a larger buffer
            exe.assign(buf.data(), static_cast<size_t>(n));
            break;
        }
        buf.resize(buf.size() * 2);
    }
#elif __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(NULL, &size); // fails, and sets size to the size needed
    if (size > 0) {
        std::vector<char> buf(size);
        if (_NSGetExecutablePath(buf.data(), &size) == 0) {
            exe = buf.data(); // NUL-terminated on success
        }
    }
#elif _WIN32
    std::vector<char> buf(MAX_PATH);
    for (;;) {
        const DWORD n = GetModuleFileNameA(NULL, buf.data(), static_cast<DWORD>(buf.size()));
        if (n == 0) {
            break;
        }
        if (n < buf.size()) { // n == buf.size(): truncated, try a larger buffer
            exe.assign(buf.data(), n);
            break;
        }
        buf.resize(buf.size() * 2);
    }
#endif

#ifdef _WIN32
    const std::string::size_type slash = exe.find_last_of("\\/");
#else
    const std::string::size_type slash = exe.find_last_of('/');
#endif
    if (slash == std::string::npos) {
        return std::string();
    }
    // the directory of the executable (dirname())
    std::string result = exe.substr(0, slash == 0 ? 1 : slash);
#ifdef _WIN32
    result += "\\..\\Resources\\poppler";
#else
    result += "/../Resources/poppler";
#endif
    return result;
}

// _imageLayers: the page names of document (none if it is NULL)
void
ReadPDFPlugin::setImageLayers(PopplerDocument* document)
{
    std::vector<std::string> layers = pageNames(document);
    std::lock_guard<std::mutex> lock(_imageLayersMutex);
    _imageLayers.swap(layers);
}

OfxStatus
ReadPDFPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    assert(isMultiPlanar());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    std::vector<std::string> imageLayers; // a copy, which the main thread can't change
    {
        std::lock_guard<std::mutex> lock(_imageLayersMutex);
        imageLayers = _imageLayers;
    }
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

    // the handles are released on every path, including the throws below
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

    const Arena::GObjectPtr<PopplerDocument> document = openDocument(filename);
    if (!document) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // poppler reports a page index out of range as a GLib critical: check it first
    const int nPages = poppler_document_get_n_pages(document.get());
    Arena::GObjectPtr<PopplerPage> page;
    if (layer >= 0 && layer < nPages) {
        page.reset(poppler_document_get_page(document.get(), layer));
    }
    if (!page) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read page");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // the size of this page, not of page 0, which cropped larger pages (#193)
    poppler_page_get_size(page.get(), &imageWidth, &imageHeight);

    width = Arena::toPixels(dpi * imageWidth / kPluginDPI);
    height = Arena::toPixels(dpi * imageHeight / kPluginDPI);

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

    const Arena::CairoSurfacePtr surfacePtr(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height));
    const Arena::CairoPtr crPtr(cairo_create(surfacePtr.get()));
    cairo_surface_t* const surface = surfacePtr.get();
    cairo_t* const cr = crPtr.get();

    scaleWidth = dpi/kPluginDPI;
    scaleHeight = dpi/kPluginDPI;

    cairo_scale(cr, scaleWidth, scaleHeight);

    poppler_page_render(page.get(), cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    status = cairo_status(cr);

    if (status) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    cairo_surface_flush(surface);

    unsigned char* cdata = cairo_image_surface_get_data(surface);
    std::vector<unsigned char> pixels((size_t)width * height * pixelComponentCount);
    // size_t: width * height * 4 may not fit in an int (#158)
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            for (int k = 0; k < pixelComponentCount; ++k)
                pixels[((size_t)i + (size_t)j * width) * pixelComponentCount + k] = cdata[((size_t)i + (size_t)(height - 1 - j) * width) * pixelComponentCount + k];
        }
    }

    // pixels is packed, bottom-up, and covers image: copy renderWindow as ReadSVG.cpp does (clipToFile())
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

    bool hasPage = false;
    double imageWidth, imageHeight;
    int width = 0;
    int height = 0;

    const Arena::GObjectPtr<PopplerDocument> document = openDocument(filename);
    if (!document) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // a document without pages has no page 0: check it before reading its size (and before
    // asking for it, which poppler reports as a critical error)
    const int nPages = poppler_document_get_n_pages(document.get());
    // Natron gets each page as a plane (decodePlane()), and all the planes of a clip share one RoD
    // (the RoD action has no plane argument): the bounds and the format cover every page, each at
    // (0,0) (#193). Other hosts only get page 0.
    const int sizedPages = gHostIsNatron ? nPages : std::min(nPages, 1);
    for (int i = 0; i < sizedPages; ++i) {
        const Arena::GObjectPtr<PopplerPage> page(poppler_document_get_page(document.get(), i));
        if (!page) {
            if (i == 0) {
                break; // no page 0 fails, as before
            }
            continue;
        }
        poppler_page_get_size(page.get(), &imageWidth, &imageHeight);
        const int w = Arena::toPixels(dpi * imageWidth / kPluginDPI);
        const int h = Arena::toPixels(dpi * imageHeight / kPluginDPI);
        if (i == 0) {
            hasPage = true;
            if (w <= 0 || h <= 0) {
                break; // an empty or too large page 0 fails, as before
            }
        }
        width = std::max(width, w);
        height = std::max(height, h);
    }

    if (width <= 0 || height <= 0) {
        // bounds and format were not set: report failure instead of returning garbage
        // bounds to GenericReader
        if (errorMsg) {
            *errorMsg = !hasPage ? "Failed to read page" : "Empty or too large page";
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

    const Arena::GObjectPtr<PopplerDocument> document = openDocument(filename);
    if (!document) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
    }
    setImageLayers(document.get());

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

    const Arena::GObjectPtr<PopplerDocument> document = openDocument(filename);
    if (!document) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
    }
    setImageLayers(document.get());
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
