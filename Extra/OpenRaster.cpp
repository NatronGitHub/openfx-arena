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
#include <stdint.h>
#include <zip.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <ofxNatron.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsImageEffect.h"
#include "lodepng.h"

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "OpenRaster"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.OpenRaster"
#define kPluginVersionMajor 2
#define kPluginVersionMinor 0
#define kPluginEvaluation 50

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar true

static bool gHostIsNatron = false;

class OpenRasterPlugin : public GenericReaderPlugin
{
public:
    OpenRasterPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~OpenRasterPlugin();
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
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void restoreState(const std::string& filename) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    std::string extractXML(std::string filename);
    void getImageSize(int *width, int *height, std::string filename);
    bool hasMergedImage(std::string filename);
    void getLayersInfo(xmlNode *node, std::vector<std::vector<std::string> > *layers);
    std::vector<std::vector<std::string> > imageLayers;
};

OpenRasterPlugin::OpenRasterPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles,
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

void
OpenRasterPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    assert(isMultiPlanar());
    clipComponents.addClipComponents(*_outputClip, getOutputComponents());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (imageLayers.size()>0 && gHostIsNatron) {
        for (int i = 0; i < (int)imageLayers.size(); i++) {
            std::ostringstream layerName;
            layerName << imageLayers[i][0];
            if (layerName.str().empty())
                layerName << "Image Layer #" << i; // if layer name is empty
            std::string component(kNatronOfxImageComponentsPlane);
            component.append(layerName.str());
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

void
OpenRasterPlugin::decodePlane(const std::string& filename, OfxTime /*time*/, int /*view*/, bool /*isPlayback*/, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& /*bounds*/,
                                 OFX::PixelComponentEnum /*pixelComponents*/, int pixelComponentCount, const std::string& rawComponents, int /*rowBytes*/)
{
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int layer = 0;
    if (gHostIsNatron) {
        std::string layerName;
        std::vector<std::string> layerChannels = OFX::mapPixelComponentCustomToLayerChannels(rawComponents);
        int numChannels = layerChannels.size();
        if (numChannels==5) // layer+R+G+B+A
            layerName=layerChannels[0];
        if (!layerName.empty()) {
            for (int i = 0; i < (int)imageLayers.size(); i++) {
                bool foundLayer = false;
                std::ostringstream nonameLayer;
                nonameLayer << "Image Layer #" << i; // if layer name is empty
                if (imageLayers[i][0]==layerName)
                    foundLayer = true;
                if (nonameLayer.str()==layerName && !foundLayer)
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
            pixelData[offset] = (float)pixels[offset] / 255.f;
            pixelData[offset + 1] = (float)pixels[offset + 1] / 255.f;
            pixelData[offset + 2] = (float)pixels[offset + 2] / 255.f;
            pixelData[offset + 3] = (float)pixels[offset + 3] / 255.f;
            offset += pixelComponentCount;
        }
    }

    buffer = NULL;
    delete[] pixels;
}

bool OpenRasterPlugin::getFrameBounds(const std::string& filename,
                              OfxTime /*time*/,
                              OfxRectI *bounds,
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
        *par = 1.0;
    }
    *tile_width = *tile_height = 0;
    return true;
}

void
OpenRasterPlugin::restoreState(const std::string& filename)
{
    if (!filename.empty()) {
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
                layerInfo.push_back("Default");
                layerInfo.push_back("mergedimage.png");
                imageLayers.push_back(layerInfo);
            }
            std::reverse(imageLayers.begin(),imageLayers.end());
        }
    }
    if (imageLayers.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Empty and/or corrupt image");
    }
}

void OpenRasterPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    assert(premult && components);
    restoreState(newFile);
    if (setColorSpace) {
# ifdef OFX_IO_USING_OCIO
        _ocio->setInputColorspace("sRGB");
# endif // OFX_IO_USING_OCIO
    }
    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageUnPreMultiplied;
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
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, true);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* OpenRasterPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    OpenRasterPlugin* ret =  new OpenRasterPlugin(handle, _extensions);
    ret->restoreStateFromParameters();
    return ret;
}

static OpenRasterPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
