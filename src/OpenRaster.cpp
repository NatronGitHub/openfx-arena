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
#include <stdint.h>
#include <zip.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <ofxNatron.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsImageEffect.h"
#include "ofxsMultiPlane.h"
#include "lodepng.h"

#define kPluginName "OpenRaster"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.OpenRaster"
#define kPluginVersionMajor 2
#define kPluginVersionMinor 1
#define kPluginEvaluation 50

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

class OpenRasterPlugin : public GenericReaderPlugin
{
public:
    OpenRasterPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~OpenRasterPlugin();
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
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, int view, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual bool guessParamsFromFilename(const std::string& filename, std::string *colorspace, OFX::PreMultiplicationEnum *filePremult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    virtual void changedFilename(const OFX::InstanceChangedArgs &args) OVERRIDE FINAL;
    std::string extractXML(std::string filename);
    void getImageSize(int *width, int *height, std::string filename);
    bool hasMergedImage(std::string filename);
    void getLayersInfo(xmlNode *node, std::vector<std::vector<std::string> > *layers);
    std::vector<std::vector<std::string> > imageLayers;
};

OpenRasterPlugin::OpenRasterPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles,
#ifdef OFX_EXTENSIONS_NUKE
(OFX::getImageEffectHostDescription() && OFX::getImageEffectHostDescription()->isMultiPlanar) ? kIsMultiPlanar : false
#else
false
#endif
)
{
}

OpenRasterPlugin::~OpenRasterPlugin()
{
}

std::string
OpenRasterPlugin::extractXML(std::string filename)
{
    std::string output;
    int err = 0;
    zip *fileOpen = zip_open(filename.c_str(), 0, &err);
    struct zip_stat xmlSt;
    zip_stat_init(&xmlSt);
    err=zip_stat(fileOpen,"stack.xml",0,&xmlSt);
    if (err!=-1) {
        char *xml = new char[xmlSt.size+1];
        zip_file *xmlFile = zip_fopen(fileOpen,"stack.xml",0);
        err=zip_fread(xmlFile,xml,xmlSt.size);
        if (err!=-1) {
            zip_fclose(xmlFile);
            xml[xmlSt.size] = '\0';
            output=xml;
        }
        delete[] xml;
    }
    zip_close(fileOpen);
    return output;
}

void
OpenRasterPlugin::getImageSize(int *width, int *height, std::string filename)
{
    std::string xml = extractXML(filename);
    if (!xml.empty()) {
        int imgW = 0;
        int imgH = 0;
        xmlDocPtr doc;
        doc = xmlParseDoc((const xmlChar*)xml.c_str());
        xmlNode *root_element = NULL;
        xmlNode *cur_node = NULL;
        root_element = xmlDocGetRootElement(doc);
        for (cur_node = root_element; cur_node; cur_node = cur_node->next) {
            if (cur_node->type == XML_ELEMENT_NODE) {
                if ((!xmlStrcmp(cur_node->name,(const xmlChar*)"image"))) {
                    xmlChar *imgWchar;
                    xmlChar *imgHchar;
                    imgWchar = xmlGetProp(cur_node,(const xmlChar*)"w");
                    imgHchar = xmlGetProp(cur_node,(const xmlChar*)"h");
                    imgW = atoi((const char*)imgWchar);
                    imgH = atoi((const char*)imgHchar);
                    xmlFree(imgWchar);
                    xmlFree(imgHchar);
                    if (imgW>0 && imgH>0) {
                        (*width)=imgW;
                        (*height)=imgH;
                    }
                }
            }
        }
        xmlFreeDoc(doc);
    }
}

void
OpenRasterPlugin::getLayersInfo(xmlNode *node, std::vector<std::vector<std::string> > *layers)
{
    xmlNode *cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"layer"))) {
                std::vector<std::string> layerInfo;
                xmlChar *xmlLayerName;
                xmlChar *xmlOpacity;
                xmlChar *xmlVisibility;
                xmlChar *xmlComposite;
                xmlChar *xmlPng;
                xmlChar *xmlOffsetX;
                xmlChar *xmlOffsetY;
                xmlLayerName = xmlGetProp(cur_node, (const xmlChar *)"name");
                xmlOpacity = xmlGetProp(cur_node, (const xmlChar *)"opacity"); // not used
                xmlVisibility = xmlGetProp(cur_node, (const xmlChar *)"visibility"); // not used
                xmlComposite = xmlGetProp(cur_node, (const xmlChar *)"composite-op"); // not used
                xmlPng = xmlGetProp(cur_node, (const xmlChar *)"src");
                xmlOffsetX = xmlGetProp(cur_node, (const xmlChar *)"x"); // a bit pointless since all layers are cropped in gfx app on save
                xmlOffsetY = xmlGetProp(cur_node, (const xmlChar *)"y"); // a bit pointless since all layers are cropped in gfx app on save
                std::string layerName,layerOpacity,layerVisibility,layerComposite,layerFile,layerOffsetX,layerOffsetY;

                if (xmlLayerName!=NULL)
                    layerName=(reinterpret_cast<char*>(xmlLayerName));
                if (xmlOpacity!=NULL)
                    layerOpacity=(reinterpret_cast<char*>(xmlOpacity));
                if (xmlVisibility!=NULL)
                    layerVisibility=(reinterpret_cast<char*>(xmlVisibility));
                if (xmlComposite!=NULL)
                    layerComposite=(reinterpret_cast<char*>(xmlComposite));
                if (xmlPng!=NULL)
                    layerFile=(reinterpret_cast<char*>(xmlPng));
                if (xmlOffsetX!=NULL)
                    layerOffsetX=(reinterpret_cast<char*>(xmlOffsetX));
                if (xmlOffsetY!=NULL)
                    layerOffsetY=(reinterpret_cast<char*>(xmlOffsetY));

                xmlFree(xmlLayerName);
                xmlFree(xmlOpacity);
                xmlFree(xmlVisibility);
                xmlFree(xmlComposite);
                xmlFree(xmlPng);
                xmlFree(xmlOffsetX);
                xmlFree(xmlOffsetY);

                if (layerOpacity.empty())
                    layerOpacity = "1";
                if (layerVisibility.empty())
                    layerVisibility = "1";
                else {
                    if (layerVisibility=="hidden")
                        layerVisibility = "0";
                    else
                        layerVisibility = "1";
                }
                if (layerComposite.empty())
                    layerComposite = "svg:src-over";
                if (layerOffsetX.empty())
                    layerOffsetX = "0";
                if (layerOffsetY.empty())
                    layerOffsetY = "0";
                if (!layerFile.empty() && !layerName.empty()) {
                    layerInfo.push_back(layerName);
                    layerInfo.push_back(layerFile);
                    layerInfo.push_back(layerOpacity);
                    layerInfo.push_back(layerVisibility);
                    layerInfo.push_back(layerComposite);
                    layerInfo.push_back(layerOffsetX);
                    layerInfo.push_back(layerOffsetY);
                    layers->push_back(layerInfo);
                }
            }
        }
        getLayersInfo(cur_node->children,layers);
    }
}

bool
OpenRasterPlugin::hasMergedImage(std::string filename)
{
    bool status = false;
    if (!filename.empty()) {
        int err = 0;
        zip *layerOpen = zip_open(filename.c_str(),0,&err);
        struct zip_stat layerSt;
        zip_stat_init(&layerSt);
        err=zip_stat(layerOpen,"mergedimage.png",0,&layerSt);
        if (err!=-1) {
            char *layerData = new char[layerSt.size];
            zip_file *layerFile = zip_fopen(layerOpen,"mergedimage.png",0);
            err=zip_fread(layerFile,layerData,layerSt.size);
            if (err!=-1) {
                zip_fclose(layerFile);
                if ((layerData!=NULL)&&(layerSt.size>0)) {
                    status = true;
                }
            }
            delete[] layerData;
        }
        zip_close(layerOpen);
    }
    return status;
}

OfxStatus
OpenRasterPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    assert(isMultiPlanar());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (imageLayers.size()>0 && gHostIsNatron) {
        for (int i = 0; i < (int)imageLayers.size(); i++) {
            std::string layerName;
            {
                std::ostringstream ss;
                if (!imageLayers[i][0].empty()) {
                    ss << imageLayers[i][0];
                } else {
                    ss << "Image Layer #" << i; // if layer name is empty
                }
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
OpenRasterPlugin::decodePlane(const std::string& filename, OfxTime /*time*/, int /*view*/, bool /*isPlayback*/, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& /*bounds*/,
                                 OFX::PixelComponentEnum /*pixelComponents*/, int pixelComponentCount, const std::string& rawComponents, int /*rowBytes*/)
{
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    if (imageLayers.size() == 0) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No layers");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int layer = 0;
    if (gHostIsNatron) {

        OFX::MultiPlane::ImagePlaneDesc plane, pairedPlane;
        OFX::MultiPlane::ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(rawComponents, &plane, &pairedPlane);

        if (!plane.isColorPlane()) {
            for (int i = 0; i < (int)imageLayers.size(); i++) {
                bool foundLayer = false;
                std::ostringstream nonameLayer;
                nonameLayer << "Image Layer #" << i; // if layer name is empty
                if (imageLayers[i][0]==plane.getPlaneLabel())
                    foundLayer = true;
                if (nonameLayer.str()==plane.getPlaneLabel() && !foundLayer)
                    foundLayer = true;
                if (foundLayer) {
                    layer = i;
                    break;
                }
            }
        }
        else {
            layer = 0;
        }
    }

    int width = 0;
    int height = 0;
    unsigned char* buffer = NULL;
    int renderWidth= renderWindow.x2 - renderWindow.x1;
    int renderHeight= renderWindow.y2 - renderWindow.y1;

    int err = 0;
    zip *layerOpen = zip_open(filename.c_str(),0,&err);
    struct zip_stat layerSt;
    zip_stat_init(&layerSt);
    err=zip_stat(layerOpen,imageLayers[layer][1].c_str(),0,&layerSt);
    if (err!=-1) {
        char *layerData = new char[layerSt.size];
        zip_file *layerFile = zip_fopen(layerOpen,imageLayers[layer][1].c_str(),0);
        err=zip_fread(layerFile,layerData,layerSt.size);
        if (err!=-1) {
            zip_fclose(layerFile);
            if ((layerData!=NULL)&&(layerSt.size>0)) {
                lodepng_decode32(&buffer,(unsigned int*)&width,(unsigned int*)&height,(unsigned char *)layerData,(size_t)layerSt.size);
            }
        }
        delete[] layerData;
    }
    zip_close(layerOpen);

    if (buffer==NULL || width!=renderWidth || height!=renderHeight) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    unsigned char* pixels = new unsigned char[width * height * pixelComponentCount];
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            for (int k = 0; k < pixelComponentCount; ++k)
                pixels[(i + j * width) * pixelComponentCount + k] = buffer[(i + (height - 1 - j) * width) * pixelComponentCount + k];
        }
    }

    int offset = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixelData[offset + 0] = pixels[offset + 0] * (1.f / 255);
            pixelData[offset + 1] = pixels[offset + 1] * (1.f / 255);
            pixelData[offset + 2] = pixels[offset + 2] * (1.f / 255);
            pixelData[offset + 3] = pixels[offset + 3] * (1.f / 255);
            offset += pixelComponentCount;
        }
    }

    buffer = NULL;
    delete[] pixels;
}

bool OpenRasterPlugin::getFrameBounds(const std::string& filename,
                              OfxTime /*time*/,
                                      int /*view*/,
                              OfxRectI *bounds,
                                      OfxRectI* format,
                              double *par,
                              std::string* /*error*/,int *tile_width, int *tile_height)
{
    int width = 0;
    int height = 0;
    getImageSize(&width,&height,filename);
    if (width>0 && height>0) {
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

bool OpenRasterPlugin::guessParamsFromFilename(const std::string& /*newFile*/,
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
    std::string xml = extractXML(filename);
    if (!xml.empty()) {
        xmlDocPtr doc;
        doc = xmlParseDoc((const xmlChar *)xml.c_str());
        xmlNode *root_element = NULL;
        root_element = xmlDocGetRootElement(doc);
        getLayersInfo(root_element,&imageLayers);
        xmlFreeDoc(doc);
        if (hasMergedImage(filename)) {
            std::vector<std::string> layerInfo;
            layerInfo.push_back(kFnOfxImagePlaneColour);
            layerInfo.push_back("mergedimage.png");
            imageLayers.push_back(layerInfo);
        }
        std::reverse(imageLayers.begin(),imageLayers.end());
    }
    if (imageLayers.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Empty and/or corrupt image");
    }

    *components = OFX::ePixelComponentRGBA;
    *filePremult = OFX::eImageUnPreMultiplied;

    return true;
}

void OpenRasterPlugin::changedFilename(const OFX::InstanceChangedArgs &args)
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
    std::string xml = extractXML(filename);
    if (!xml.empty()) {
        xmlDocPtr doc;
        doc = xmlParseDoc((const xmlChar *)xml.c_str());
        xmlNode *root_element = NULL;
        root_element = xmlDocGetRootElement(doc);
        getLayersInfo(root_element,&imageLayers);
        xmlFreeDoc(doc);
        if (hasMergedImage(filename)) {
            std::vector<std::string> layerInfo;
            layerInfo.push_back(kFnOfxImagePlaneColour);
            layerInfo.push_back("mergedimage.png");
            imageLayers.push_back(layerInfo);
        }
        std::reverse(imageLayers.begin(),imageLayers.end());
    }
}

void OpenRasterPlugin::restoreStateFromParams()
{
    GenericReaderPlugin::restoreStateFromParams();

    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st == kOfxStatOK || !filename.empty() ) {
        imageLayers.clear();
        std::string xml = extractXML(filename);
        if (!xml.empty()) {
            xmlDocPtr doc;
            doc = xmlParseDoc((const xmlChar *)xml.c_str());
            xmlNode *root_element = NULL;
            root_element = xmlDocGetRootElement(doc);
            getLayersInfo(root_element,&imageLayers);
            xmlFreeDoc(doc);
            if (hasMergedImage(filename)) {
                std::vector<std::string> layerInfo;
                layerInfo.push_back(kFnOfxImagePlaneColour);
                layerInfo.push_back("mergedimage.png");
                imageLayers.push_back(layerInfo);
            }
            std::reverse(imageLayers.begin(),imageLayers.end());
        }
    }
}

using namespace OFX;

mDeclareReaderPluginFactory(OpenRasterPluginFactory, {}, false);

void
OpenRasterPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("ora");
}

/** @brief The basic describe function, passed a plugin descriptor */
void OpenRasterPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, kIsMultiPlanar);
    desc.setLabel(kPluginName);
    desc.setPluginDescription("Read OpenRaster image format.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void OpenRasterPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsXY,kSupportsAlpha, kSupportsTiles, true);
    GenericReaderDescribeInContextEnd(desc, context, page, "sRGB", "scene_linear");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* OpenRasterPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    OpenRasterPlugin* ret =  new OpenRasterPlugin(handle, _extensions);
    ret->restoreStateFromParams();
    return ret;
}

static OpenRasterPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
