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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>

#include "ofxNatron.h"
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsMultiPlane.h"
#include "ofxsImageEffect.h"

#define kPluginName "ReadSVG"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadSVG"
#define kPluginVersionMajor 3
#define kPluginVersionMinor 2
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
#define kIsMultiPlanar true

using namespace OFX::IO;

#ifdef OFX_IO_USING_OCIO
namespace OCIO = OCIO_NAMESPACE;
#endif

OFXS_NAMESPACE_ANONYMOUS_ENTER

static bool gHostIsNatron = false;

class ReadSVGPlugin : public GenericReaderPlugin
{
public:
    ReadSVGPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadSVGPlugin();
    virtual void restoreStateFromParams() OVERRIDE FINAL;
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
    virtual OfxStatus getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual bool guessParamsFromFilename(const std::string& filename, std::string *colorspace, OFX::PreMultiplicationEnum *filePremult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    virtual void changedFilename(const OFX::InstanceChangedArgs &args) OVERRIDE FINAL;
    void getLayers(xmlNode *node, std::vector<std::string> *layers);
    OFX::IntParam *_dpi;
    std::vector<std::string> imageLayers;
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
}

ReadSVGPlugin::~ReadSVGPlugin()
{
}

void ReadSVGPlugin::restoreStateFromParams()
{
    GenericReaderPlugin::restoreStateFromParams();

    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st == kOfxStatOK || !filename.empty() ) {
        imageLayers.clear();
        xmlDocPtr doc;
        doc = xmlParseFile(filename.c_str());
        xmlNode *root_element = NULL;
        root_element = xmlDocGetRootElement(doc);
        getLayers(root_element,&imageLayers);
        xmlFreeDoc(doc);
    }
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

OfxStatus
ReadSVGPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    assert(isMultiPlanar());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (imageLayers.size()>0 && gHostIsNatron) {
        for (int i = 0; i < (int)imageLayers.size(); i++) {
            if (!imageLayers[i].empty()) {

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
        }
    }
    return kOfxStatOK;
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

    std::string layerID;
    if (gHostIsNatron) {
        OFX::MultiPlane::ImagePlaneDesc plane, pairedPlane;
        OFX::MultiPlane::ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(rawComponents, &plane, &pairedPlane);
        layerID = plane.getPlaneLabel();
        if (plane.isColorPlane()) {
            layerID.clear();
        }
    }

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
    handle=rsvg_handle_new_from_file(filename.c_str(), &error);

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

    if (layerID.empty()) {
        rsvg_handle_render_cairo(handle, cr);
    }
    else {
        std::ostringstream layerSub;
        layerSub << "#" << layerID;
        rsvg_handle_render_cairo_sub(handle, cr, layerSub.str().c_str());
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

    g_object_unref(handle);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cdata = NULL;
    error = NULL;
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

    int dpi;
    _dpi->getValueAtTime(time, dpi);

    GError *error = NULL;
    RsvgHandle *handle;
    RsvgDimensionData dimension;
    double imageWidth, imageHeight;
    int width, height;

    rsvg_set_default_dpi_x_y(dpi, dpi);

    handle=rsvg_handle_new_from_file(filename.c_str(), &error);

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

bool ReadSVGPlugin::guessParamsFromFilename(const std::string& /*newFile*/,
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

    imageLayers.clear();
    xmlDocPtr doc;
    doc = xmlParseFile(filename.c_str());
    xmlNode *root_element = NULL;
    root_element = xmlDocGetRootElement(doc);
    getLayers(root_element,&imageLayers);
    xmlFreeDoc(doc);

    GError *error = NULL;
    RsvgHandle *handle;

    handle=rsvg_handle_new_from_file(filename.c_str(), &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    g_object_unref(handle);
    error = NULL;

    *components = OFX::ePixelComponentRGBA;
    *filePremult = OFX::eImageUnPreMultiplied;

    return true;
}

void ReadSVGPlugin::changedFilename(const OFX::InstanceChangedArgs &args)
{
    GenericReaderPlugin::changedFilename(args);

    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st != kOfxStatOK || filename.empty() ) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    imageLayers.clear();
    xmlDocPtr doc;
    doc = xmlParseFile(filename.c_str());
    xmlNode *root_element = NULL;
    root_element = xmlDocGetRootElement(doc);
    getLayers(root_element,&imageLayers);
    xmlFreeDoc(doc);
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadSVGPluginFactory, {}, false);

void
ReadSVGPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("svg");
    _extensions.push_back("svgz");
}


/** @brief The basic describe function, passed a plugin descriptor */
void ReadSVGPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, kIsMultiPlanar);
    desc.setLabel(kPluginName);
    desc.setPluginDescription("Fast SVG (Scalable Vector Graphics) reader using librsvg and Cairo.");
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

    GenericReaderDescribeInContextEnd(desc, context, page, "sRGB", "scene_linear");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadSVGPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadSVGPlugin* ret =  new ReadSVGPlugin(handle, _extensions);
    ret->restoreStateFromParams();
    return ret;
}

static ReadSVGPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
