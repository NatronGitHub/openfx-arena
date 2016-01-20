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

#include <iostream>
#include <stdint.h>
#include <zip.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsImageEffect.h"

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "OpenRasterOFX"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.OpenRaster"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define OpenRasterVersion 0.0.5

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

class OpenRasterPlugin : public GenericReaderPlugin
{
public:
    OpenRasterPlugin(OfxImageEffectHandle handle);
    virtual ~OpenRasterPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    std::string extractXML(std::string filename);
    void getImageSize(int *width, int *height, std::string filename);
    std::string getImageVersion(std::string filename);
    Magick::Image getImage(std::string filename);
    Magick::Image getLayer(std::string filename, std::string layerfile);
    void getLayersSpecs(xmlNode *node, std::vector<std::vector<std::string> > *layers);
    void setupImage(std::string filename, std::vector<Magick::Image> *images);
    //std::vector<Magick::Image> image;
};

OpenRasterPlugin::OpenRasterPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
{
    Magick::InitializeMagick(NULL);
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
        char *xml = new char[xmlSt.size];
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
#ifdef DEBUG
    std::cout << output << std::endl;
#endif
    return output;
}

std::string
OpenRasterPlugin::getImageVersion(std::string filename)
{
    std::string output;
    std::string xml = extractXML(filename);
    if (!xml.empty()) {
        xmlDocPtr doc;
        doc = xmlParseDoc((const xmlChar*)xml.c_str());
        xmlNode *root_element = NULL;
        xmlNode *cur_node = NULL;
        root_element = xmlDocGetRootElement(doc);
        for (cur_node = root_element; cur_node; cur_node = cur_node->next) {
            if (cur_node->type == XML_ELEMENT_NODE) {
                if ((!xmlStrcmp(cur_node->name,(const xmlChar*)"image"))) {
                    xmlChar *imgVersion;
                    imgVersion = xmlGetProp(cur_node,(const xmlChar*)"version");
                    output = (const char*)imgVersion;
                    xmlFree(imgVersion);
                }
            }
        }
        xmlFreeDoc(doc);
    }
#ifdef DEBUG
    std::cout << "Image version: " << output << std::endl;
#endif
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
#ifdef DEBUG
                        std::cout << "Image size: " << imgW << "x" << imgH << std::endl;
#endif
                    }
                }
            }
        }
        xmlFreeDoc(doc);
    }
}

void
OpenRasterPlugin::getLayersSpecs(xmlNode *node, std::vector<std::vector<std::string> > *layers)
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
                xmlOpacity = xmlGetProp(cur_node, (const xmlChar *)"opacity");
                xmlVisibility = xmlGetProp(cur_node, (const xmlChar *)"visibility");
                xmlComposite = xmlGetProp(cur_node, (const xmlChar *)"composite-op");
                xmlPng = xmlGetProp(cur_node, (const xmlChar *)"src");
                xmlOffsetX = xmlGetProp(cur_node, (const xmlChar *)"x");
                xmlOffsetY = xmlGetProp(cur_node, (const xmlChar *)"y");
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
                    layerInfo.push_back(layerOpacity);
                    layerInfo.push_back(layerVisibility);
                    layerInfo.push_back(layerComposite);
                    layerInfo.push_back(layerOffsetX);
                    layerInfo.push_back(layerOffsetY);
                    layerInfo.push_back(layerFile);
                    layers->push_back(layerInfo);
                }
            }
        }
        getLayersSpecs(cur_node->children,layers);
    }
}

void
OpenRasterPlugin::setupImage(std::string filename, std::vector<Magick::Image> *images)
{
    if (!filename.empty()) {
        std::string xml = extractXML(filename);
        if (!xml.empty()) {
            std::vector<std::vector<std::string> > layersInfo;
            xmlDocPtr doc;
            doc = xmlParseDoc((const xmlChar *)xml.c_str());
            xmlNode *root_element = NULL;
            root_element = xmlDocGetRootElement(doc);
            getLayersSpecs(root_element,&layersInfo);
            xmlFreeDoc(doc);
            if (!layersInfo.empty()) {
                std::reverse(layersInfo.begin(),layersInfo.end());
                for (int i = 0; i < (int)layersInfo.size(); i++) {
                    Magick::Image layer;
                    layer=getLayer(filename,layersInfo[i][6]);
                    if (layer.format()=="Portable Network Graphics" && layer.columns()>0 && layer.rows()>0) {
                        int xOffset = atoi(layersInfo[i][4].c_str());
                        int yOffset = atoi(layersInfo[i][5].c_str());
                        layer.label(layersInfo[i][0]);
                        std::string comment = layersInfo[i][3] + " " + layersInfo[i][1] + " " + layersInfo[i][2];
                        layer.comment(comment);
                        layer.page().xOff(xOffset);
                        layer.page().yOff(yOffset);
                        images->push_back(layer);
                    }
#ifdef DEBUG
                    std::cout << "Layer: " << layersInfo[i][0] << " " << layersInfo[i][1] << " " << layersInfo[i][2] << " " << layersInfo[i][3] << " " << layersInfo[i][4] << " " << layersInfo[i][5] << " " << layersInfo[i][6] << std::endl;
#endif
                }
            }
        }
    }
}

Magick::Image
OpenRasterPlugin::getImage(std::string filename)
{
    Magick::Image image;
    int err = 0;
    zip *imageOpen = zip_open(filename.c_str(),0,&err);
    struct zip_stat imageSt;
    zip_stat_init(&imageSt);
    err=zip_stat(imageOpen,"mergedimage.png",0,&imageSt);
    if (err!=-1) {
        char *imageData = new char[imageSt.size];
        zip_file *imageFile = zip_fopen(imageOpen,"mergedimage.png",0);
        err=zip_fread(imageFile,imageData,imageSt.size);
        if (err!=-1) {
            zip_fclose(imageFile);
            if ((imageData!=NULL) && (imageSt.size>0)) {
                Magick::Blob blob(imageData,imageSt.size);
                Magick::Image tmp(blob);
                if (tmp.format()=="Portable Network Graphics")
                    image=tmp;
            }
        }
        delete[] imageData;
    }
    zip_close(imageOpen);
    return image;
}

Magick::Image
OpenRasterPlugin::getLayer(std::string filename, std::string layer)
{
    Magick::Image image;
    int err = 0;
    zip *layerOpen = zip_open(filename.c_str(),0,&err);
    struct zip_stat layerSt;
    zip_stat_init(&layerSt);
    err=zip_stat(layerOpen,layer.c_str(),0,&layerSt);
    if (err!=-1) {
        char *layerData = new char[layerSt.size];
        zip_file *layerFile = zip_fopen(layerOpen,layer.c_str(),0);
        err=zip_fread(layerFile,layerData,layerSt.size);
        if (err!=-1) {
            zip_fclose(layerFile);
            if ((layerData!=NULL)&&(layerSt.size>0)) {
                Magick::Blob blob(layerData,layerSt.size);
                Magick::Image tmp(blob);
                if (tmp.format()=="Portable Network Graphics")
                    image=tmp;
            }
        }
        delete[] layerData;
    }
    zip_close(layerOpen);
    return image;
}

void
OpenRasterPlugin::decode(const std::string& filename,
                      OfxTime /*time*/,
                      int /*view*/,
                      bool /*isPlayback*/,
                      const OfxRectI& renderWindow,
                      float *pixelData,
                      const OfxRectI& bounds,
                      OFX::PixelComponentEnum /*pixelComponents*/,
                      int /*pixelComponentCount*/,
                      int /*rowBytes*/)
{
#ifdef DEBUG
    getImageVersion(filename);
    std::vector<Magick::Image> _image;
    setupImage(filename,&_image);
#endif

    Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image image(getImage(filename));
    if ((int)image.columns()==bounds.x2 && (int)image.rows()==bounds.y2) {
        container.composite(image,0,0,Magick::OverCompositeOp);
        container.flip();
        container.write(0,0,renderWindow.x2 - renderWindow.x1,renderWindow.y2 - renderWindow.y1,"RGBA",Magick::FloatPixel,pixelData);
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
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

void OpenRasterPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    assert(premult && components);
    int width = 0;
    int height = 0;
    getImageSize(&width,&height,newFile);
    if (width>0 && height>0) {
        if (setColorSpace) {
            # ifdef OFX_IO_USING_OCIO
            _ocio->setInputColorspace("sRGB");
            # endif // OFX_IO_USING_OCIO
        }
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageUnPreMultiplied;
}

using namespace OFX;

mDeclareReaderPluginFactory(OpenRasterPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void OpenRasterPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"ora", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(50);
    #endif

    desc.setPluginDescription("Read OpenRaster image format.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void OpenRasterPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* OpenRasterPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    OpenRasterPlugin* ret =  new OpenRasterPlugin(handle);
    ret->restoreStateFromParameters();
    return ret;
}

static OpenRasterPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
