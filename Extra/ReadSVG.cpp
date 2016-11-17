/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
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

#include <librsvg/rsvg.h>
#ifdef LEGACY
#include "librsvg/rsvg-cairo.h"
#endif
#include <librevenge/librevenge.h>
#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>
#include <libcdr/libcdr.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <poppler.h>
#include <poppler/GlobalParams.h>

#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

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

#include "ofxNatron.h"
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsImageEffect.h"

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadSVG"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadSVG"
#define kPluginVersionMajor 4
#define kPluginVersionMinor 0
#define kPluginEvaluation 50
#define kPDFDPI 72.0

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch (90 is default)"
#define kParamDpiDefault 90

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsXY false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar true

using namespace OFX::IO;

static bool gHostIsNatron = false;

class ReadSVGPlugin : public GenericReaderPlugin
{
public:
    ReadSVGPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadSVGPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds,
                             OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL
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
        decodePlane(filename, time, view, isPlayback, renderWindow, pixelData, bounds, pixelComponents, pixelComponentCount, rawComps, rowBytes);
    }
    virtual void decodePlane(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, const std::string& rawComponents, int rowBytes) OVERRIDE FINAL;
    virtual void getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void restoreState(const std::string& filename) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool throwErrors, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    void getLayers(xmlNode *node, std::vector<std::string> *layers);
    OFX::IntParam *_dpi;
    std::vector<std::string> imageLayers;
    bool isSVG(const std::string &filename);
    bool isPDF(const std::string &filename);
    bool isCDR(const std::string &filename);
    std::string getPopplerPath();
};

ReadSVGPlugin::ReadSVGPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles,
#ifdef OFX_EXTENSIONS_NUKE
(OFX::getImageEffectHostDescription() && OFX::getImageEffectHostDescription()->isMultiPlanar) ? kIsMultiPlanar : false
#else
false
#endif
)
,_dpi(0)
{
    _dpi = fetchIntParam(kParamDpi);
    assert(_dpi);

    // When we deploy the plugin we need access to the poppler data folder,
    // we asume that the path is OFX_HOST_BINARY/../Resources/poppler.
    // If not found, whatever poppler has as default will be used (may or may not work).
    std::string popplerData = getPopplerPath();
    if (!popplerData.empty()) {
        struct stat sb;
        if (stat(popplerData.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
            globalParams = new GlobalParams(popplerData.c_str());
        }
    }
}

ReadSVGPlugin::~ReadSVGPlugin()
{
}

void
ReadSVGPlugin::getLayers(xmlNode *node, std::vector<std::string> *layers)
{
    xmlNode *cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"g"))) {
                xmlChar *xmlID;
                xmlID = xmlGetProp(cur_node, (const xmlChar *)"id");
                if (xmlID!=NULL) {
                    std::string layerName;
                    layerName = (reinterpret_cast<char*>(xmlID));
                    layers->push_back(layerName);
                }
                xmlFree(xmlID);
            }
            else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"path"))) {
                xmlChar *xmlID;
                xmlID = xmlGetProp(cur_node, (const xmlChar *)"id");
                if (xmlID!=NULL) {
                    std::string layerName;
                    layerName = (reinterpret_cast<char*>(xmlID));
                    layers->push_back(layerName);
                }
                xmlFree(xmlID);
            }
        }
        getLayers(cur_node->children,layers);
    }
}

void
ReadSVGPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    assert(isMultiPlanar());
    clipComponents.addClipComponents(*_outputClip, getOutputComponents());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (imageLayers.size()>0 && gHostIsNatron) {
        for (int i = 0; i < (int)imageLayers.size(); i++) {
            if (!imageLayers[i].empty()) {
                std::string component(kNatronOfxImageComponentsPlane);
                component.append(imageLayers[i]);
                component.append(kNatronOfxImageComponentsPlaneChannel);
                component.append("R");
                component.append(kNatronOfxImageComponentsPlaneChannel);
                component.append("G");
                component.append(kNatronOfxImageComponentsPlaneChannel);
                component.append("B");
                component.append(kNatronOfxImageComponentsPlaneChannel);
                component.append("A");
                clipComponents.addClipComponents(*_outputClip, component);
            }
        }
    }
}

void
ReadSVGPlugin::decodePlane(const std::string& filename, OfxTime time, int /*view*/, bool /*isPlayback*/, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& /*bounds*/,
                                 OFX::PixelComponentEnum /*pixelComponents*/, int pixelComponentCount, const std::string& rawComponents, int /*rowBytes*/)
{
    if (pixelComponentCount != 4) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Wrong pixel components");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int layer = 0;
    std::string layerID;
    if (gHostIsNatron) {
        std::string layerName;
        std::vector<std::string> layerChannels = OFX::mapPixelComponentCustomToLayerChannels(rawComponents);
        int numChannels = layerChannels.size();
        if (numChannels==5) // layer+R+G+B+A
            layerName=layerChannels[0];
        if (!layerName.empty()) {
            layerID = layerName;
            layer = atoi(layerName.c_str());
        }
    }

    double imageWidth, imageHeight, scaleWidth, scaleHeight;
    int dpi, width, height;
    _dpi->getValueAtTime(time, dpi);
    bool pdf = isPDF(filename);
    //bool svg = isSVG(filename);
    bool cdr = isCDR(filename);
    int renderWidth = renderWindow.x2 - renderWindow.x1;
    int renderHeight = renderWindow.y2 - renderWindow.y1;

    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_status_t status;

    if(pdf) {
        GError *error = NULL;
        PopplerDocument *document = NULL;
        PopplerPage *page = NULL;
        PopplerPage *fpage = NULL;

        gchar *absolute, *uri;
        absolute = g_strdup(filename.c_str());
        uri = g_filename_to_uri(absolute, NULL, &error);
        free(absolute);

        document = poppler_document_new_from_file(uri, NULL, &error);

        if (error != NULL) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
            OFX::throwSuiteStatusException(kOfxStatErrFormat);
        }

        page = poppler_document_get_page(document, layer);
        fpage = poppler_document_get_page(document, 0);

        if (page == NULL || fpage == NULL) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF page");
            OFX::throwSuiteStatusException(kOfxStatErrFormat);
        }

        poppler_page_get_size(fpage, &imageWidth, &imageHeight);

        width = dpi * imageWidth / kPDFDPI;
        height = dpi * imageHeight / kPDFDPI;
        scaleWidth = dpi/kPDFDPI;
        scaleHeight = dpi/kPDFDPI;

        if (width != renderWidth || height != renderHeight) {
            setPersistentMessage(OFX::Message::eMessageError, "", "PDF don't match RenderWindow");
            OFX::throwSuiteStatusException(kOfxStatErrFormat);
        }

        surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr=cairo_create(surface);
        cairo_scale(cr, scaleWidth, scaleHeight);
        poppler_page_render(page, cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);

        g_object_unref(page);
        g_object_unref(fpage);
        g_object_unref(document);
        error = NULL;
    } else {
        std::ostringstream stream;
        if (cdr) {
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
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
            for (unsigned k = 0; k<output.size(); ++k) {
                stream << output[k].cstr();
            }
        }
        GError *error = NULL;
        RsvgHandle *handle;
        RsvgDimensionData dimension;

        rsvg_set_default_dpi_x_y(dpi, dpi);
        if (cdr) {
            handle=rsvg_handle_new_from_data((guint8 *)stream.str().c_str(), stream.str().size(), &error);
        } else {
            handle=rsvg_handle_new_from_file(filename.c_str(), &error);
        }

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
        } else {
            width = imageWidth;
            height = imageHeight;
        }

        scaleWidth = width / imageWidth;
        scaleHeight = height / imageHeight;

        if (width != renderWidth || height != renderHeight) {
            setPersistentMessage(OFX::Message::eMessageError, "", "SVG don't match RenderWindow");
            OFX::throwSuiteStatusException(kOfxStatErrFormat);
        }

        surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr=cairo_create(surface);
        cairo_scale(cr, scaleWidth, scaleHeight);

        if (layerID.empty()) {
            rsvg_handle_render_cairo(handle, cr);
        } else {
            std::ostringstream layerSub;
            layerSub << "#" << layerID;
            rsvg_handle_render_cairo_sub(handle, cr, layerSub.str().c_str());
        }

        g_object_unref(handle);
        error = NULL;
    }

    status = cairo_status(cr);
    if (status) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Cairo Render failed");
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

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cdata = NULL;
    delete[] pixels;
}

bool ReadSVGPlugin::getFrameBounds(const std::string& filename,
                              OfxTime time,
                              OfxRectI *bounds,
                                   OfxRectI* format,
                              double *par,
                              std::string* /*error*/,int *tile_width, int *tile_height)
{
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int dpi, width, height;
    _dpi->getValueAtTime(time, dpi);
    bool cdr = isCDR(filename);

    if (isPDF(filename)) {
        GError *error = NULL;
        PopplerDocument *document = NULL;
        PopplerPage *page = NULL;
        double imageWidth, imageHeight;

        gchar *absolute, *uri;
        absolute = g_strdup(filename.c_str());
        uri = g_filename_to_uri(absolute, NULL, &error);
        free(absolute);

        document = poppler_document_new_from_file(uri, NULL, &error);

        if (error != NULL) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
            OFX::throwSuiteStatusException(kOfxStatErrFormat);
        }

        page = poppler_document_get_page(document, 0);
        poppler_page_get_size(page, &imageWidth, &imageHeight);
        if (page !=NULL) {
            width = dpi * imageWidth / kPDFDPI;
            height = dpi * imageHeight / kPDFDPI;
        }

        g_object_unref(page);
        g_object_unref(document);
        error = NULL;
    } else {
        std::ostringstream stream;
        if (cdr) {
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
            stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
            for (unsigned k = 0; k<output.size(); ++k) {
                stream << output[k].cstr();
            }
        }
        GError *error = NULL;
        RsvgHandle *handle;
        RsvgDimensionData dimension;
        double imageWidth, imageHeight;

        rsvg_set_default_dpi_x_y(dpi, dpi);
        if (cdr) {
            handle=rsvg_handle_new_from_data((guint8 *)stream.str().c_str(), stream.str().size(), &error);
        } else {
            handle=rsvg_handle_new_from_file(filename.c_str(), &error);
        }

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
        } else {
            width = imageWidth;
            height = imageHeight;
        }

        g_object_unref(handle);
        error = NULL;
    }

    if (width > 0 && height > 0) {
        bounds->x1 = 0;
        bounds->x2 = width;
        bounds->y1 = 0;
        bounds->y2 = height;
        *format = *bounds;
        *par = 1.0;
    }

    *tile_width = *tile_height = 0;
    return true;
}

void
ReadSVGPlugin::restoreState(const std::string& filename)
{
    if (!filename.empty()) {
        bool cdr = isCDR(filename);
        imageLayers.clear();
        if (isPDF(filename)) {
            GError *error = NULL;
            PopplerDocument *document = NULL;
            gchar *absolute, *uri;
            absolute = g_strdup(filename.c_str());
            uri = g_filename_to_uri(absolute, NULL, &error);
            free(absolute);
            document = poppler_document_new_from_file(uri, NULL, &error);
            if (error != NULL) {
                setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read PDF");
            } else {
                imageLayers.clear();
                int pages = 0;
                pages = poppler_document_get_n_pages(document);
                if (pages<0) {
                    pages=0;
                }
                for (int i = 0; i < pages; i++) {
                    std::ostringstream pageName;
                    pageName << i;
                    imageLayers.push_back(pageName.str());
                }
            }
            g_object_unref(document);
            error = NULL;
        } else {
            std::ostringstream stream;
            if (cdr) {
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
                stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
                for (unsigned k = 0; k<output.size(); ++k) {
                    stream << output[k].cstr();
                }
            } else  {
                xmlDocPtr doc;
                doc = xmlParseFile(filename.c_str());
                xmlNode *root_element = NULL;
                root_element = xmlDocGetRootElement(doc);
                getLayers(root_element,&imageLayers);
                xmlFreeDoc(doc);
            }
            GError *error = NULL;
            RsvgHandle *handle;
            if (cdr) {
                handle=rsvg_handle_new_from_data((guint8 *)stream.str().c_str(), stream.str().size(), &error);
            } else {
                handle=rsvg_handle_new_from_file(filename.c_str(), &error);
            }
            if (error != NULL) {
                setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
            }
            g_object_unref(handle);
            error = NULL;
        }
    } else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Empty and/or corrupt image");
    }
}

void ReadSVGPlugin::onInputFileChanged(const std::string& newFile,
                                  bool /*throwErrors*/,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    assert(premult && components);
    restoreState(newFile);
    if (setColorSpace) {
# ifdef OFX_IO_USING_OCIO
        _ocio->setInputColorspace("sRGB");
# endif
    }

    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageUnPreMultiplied;
}

bool
ReadSVGPlugin::isSVG(const std::string &filename)
{
    bool result = false;
    int suffixSize = 3;
    if (filename.size() > suffixSize) {
        std::string vectorFile = filename;
        std::transform(vectorFile.begin(), vectorFile.end(), vectorFile.begin(), ::tolower);
        bool hasSVG = vectorFile.compare(vectorFile.size()-suffixSize,suffixSize, "svg") == 0;
        bool hasSVGZ = vectorFile.compare(vectorFile.size()-suffixSize+1,suffixSize+1, "svgz") == 0;
        if (hasSVG || hasSVGZ) {
            result = true;
        }
    }
    return result;
}

bool
ReadSVGPlugin::isPDF(const std::string &filename)
{
    bool result = false;
    int suffixSize = 3;
    if (filename.size() > suffixSize) {
        std::string vectorFile = filename;
        std::transform(vectorFile.begin(), vectorFile.end(), vectorFile.begin(), ::tolower);
        result = vectorFile.compare(vectorFile.size()-suffixSize,suffixSize, "pdf") == 0;
    }
    return result;
}

bool
ReadSVGPlugin::isCDR(const std::string &filename)
{
    bool result = false;
    int suffixSize = 3;
    if (filename.size() > suffixSize) {
        std::string vectorFile = filename;
        std::transform(vectorFile.begin(), vectorFile.end(), vectorFile.begin(), ::tolower);
        result = vectorFile.compare(vectorFile.size()-suffixSize,suffixSize, "cdr") == 0;
    }
    return result;
}

std::string
ReadSVGPlugin::getPopplerPath()
{
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

using namespace OFX;

mDeclareReaderPluginFactory(ReadSVGPluginFactory, {}, false);

void
ReadSVGPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("svg");
    _extensions.push_back("svgz");
    _extensions.push_back("pdf");
    //_extensions.push_back("cdr");
}


/** @brief The basic describe function, passed a plugin descriptor */
void ReadSVGPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, kIsMultiPlanar);
    desc.setLabel(kPluginName);
    desc.setPluginDescription("Vector graphics reader.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadSVGPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsXY,kSupportsAlpha, kSupportsTiles, false);
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamDpi);
        param->setLabel(kParamDpiLabel);
        param->setHint(kParamDpiHint);
        param->setRange(1, 10000);
        param->setDisplayRange(1, 500);
        param->setDefault(kParamDpiDefault);
        param->setAnimates(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }

    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadSVGPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadSVGPlugin* ret =  new ReadSVGPlugin(handle, _extensions);
    ret->restoreStateFromParameters();
    return ret;
}

static ReadSVGPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
