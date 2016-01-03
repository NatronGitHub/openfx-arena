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

#include "ReadORA.h"
#include <iostream>
#include <stdint.h>

#include <zip.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <Magick++.h>

#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadORA"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadORA"
#define kPluginVersionMajor 0
#define kPluginVersionMinor 5

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

std::string _extractXML(std::string oraFile)
{
    std::string output;

    int err = 0;
    zip *oraOpen = zip_open(oraFile.c_str(), 0, &err);

    const char *xmlName = "stack.xml";
    struct zip_stat xmlSt;
    zip_stat_init(&xmlSt);
    err=zip_stat(oraOpen, xmlName, 0, &xmlSt);

    if (err!=-1) {
        char *xml = new char[xmlSt.size];
        zip_file *xmlFile = zip_fopen(oraOpen, "stack.xml", 0);
        err=zip_fread(xmlFile, xml, xmlSt.size);
        if (err!=-1) {
            zip_fclose(xmlFile);
            xml[xmlSt.size] = '\0';
            output = xml;
        }
    }
    zip_close(oraOpen);

    return output;
}

void _getImageSize(int *width, int *height, std::string filename)
{
    std::string xml = _extractXML(filename);
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

class ReadORAPlugin : public GenericReaderPlugin
{
public:
    ReadORAPlugin(OfxImageEffectHandle handle);
    virtual ~ReadORAPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;

    void setupImage(std::string filename);
    void getLayersInfo(xmlNode *node);
    Magick::Image getLayer(std::string filename, std::string layerfile);
    std::vector<Magick::Image> _image;
    std::vector<std::vector<std::string> > _layersInfo;
};

ReadORAPlugin::ReadORAPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
{
    Magick::InitializeMagick(NULL);
}

ReadORAPlugin::~ReadORAPlugin()
{
}

void
ReadORAPlugin::getLayersInfo(xmlNode *node)
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

                if (layerName.empty())
                    layerName = "unnamed";
                if (layerOpacity.empty())
                    layerOpacity = "1";
                if (layerVisibility.empty())
                    layerVisibility = "visible";
                if (layerComposite.empty())
                    layerComposite = "svg:src-over";
                if (layerOffsetX.empty())
                    layerOffsetX = "0";
                if (layerOffsetY.empty())
                    layerOffsetY = "0";
                if (!layerFile.empty()) {
                    layerInfo.push_back(layerName);
                    layerInfo.push_back(layerOpacity);
                    layerInfo.push_back(layerVisibility);
                    layerInfo.push_back(layerComposite);
                    layerInfo.push_back(layerOffsetX);
                    layerInfo.push_back(layerOffsetY);
                    layerInfo.push_back(layerFile);
                    _layersInfo.push_back(layerInfo);
                }
            }
        }
        getLayersInfo(cur_node->children);
    }
}

Magick::Image
ReadORAPlugin::getLayer(std::string filename, std::string layer)
{
    Magick::Image image;
    int err = 0;
    zip *layerOpen = zip_open(filename.c_str(), 0, &err);
    const char *layerPath = layer.c_str();
    struct zip_stat layerSt;
    zip_stat_init(&layerSt);
    err=zip_stat(layerOpen, layerPath, 0, &layerSt);
    if (err!=-1) {
        char *layerData = new char[layerSt.size];
        zip_file *layerFile = zip_fopen(layerOpen, layer.c_str(), 0);
        err=zip_fread(layerFile, layerData, layerSt.size);
        if (err!=-1) {
            zip_fclose(layerFile);
            zip_close(layerOpen);
            if ((layerData!=NULL)&&(layerSt.size>0)) {
                Magick::Blob blob(layerData,layerSt.size);
                Magick::Image tmp(blob);
                if (tmp.format()=="Portable Network Graphics")
                    image=tmp;
            }
        }
    }
    return image;
}

void
ReadORAPlugin::setupImage(std::string filename)
{
    if (!filename.empty()) {
        std::string xml = _extractXML(filename);
        if (!xml.empty()) {
            _image.clear();
            _layersInfo.clear();
            xmlDocPtr doc;
            doc = xmlParseDoc((const xmlChar *)xml.c_str());
            xmlNode *root_element = NULL;
            root_element = xmlDocGetRootElement(doc);
            getLayersInfo(root_element);
            xmlFreeDoc(doc);
            if (!_layersInfo.empty()) {
                std::reverse(_layersInfo.begin(),_layersInfo.end());
                for (int i = 0; i < (int)_layersInfo.size(); i++) {
                    Magick::Image layer;
                    layer=getLayer(filename,_layersInfo[i][6]);
                    if (layer.format()=="Portable Network Graphics" && layer.columns()>0 && layer.rows()>0) {
                        int xOffset = atoi(_layersInfo[i][4].c_str());
                        int yOffset = atoi(_layersInfo[i][5].c_str());
                        layer.label(_layersInfo[i][6]); // use filename so we can get layerinfo using that later, since layername may not be uniqe
                        layer.page().xOff(xOffset);
                        layer.page().yOff(yOffset);
                        _image.push_back(layer);
                    }
                }
            }
        }
    }
}

void
ReadORAPlugin::decode(const std::string& filename,
                      OfxTime time,
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
    std::cout << "decode ..." << std::endl;
    #endif

    // TODO!!!

    int width = 0;
    int height = 0;
    if (!filename.empty())
        _getImageSize(&width,&height,filename);
    if (width>0 && height>0) {
        setupImage(filename); // move to restorestate etc
        Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,1)"));

        //testing (need to check if layer should be visible, also need to use right composite-op, and of course multiplane support)
        Magick::Image image(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
        if (!_image.empty()) {
            for (int i = 0; i < _image.size(); i++)
                image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::OverCompositeOp);
        }

        container.composite(image,0,0,Magick::OverCompositeOp);
        container.composite(image,0,0,Magick::CopyOpacityCompositeOp);
        container.flip();
        container.write(0,0,renderWindow.x2 - renderWindow.x1,renderWindow.y2 - renderWindow.y1,"RGBA",Magick::FloatPixel,pixelData);
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

bool ReadORAPlugin::getFrameBounds(const std::string& filename,
                              OfxTime /*time*/,
                              OfxRectI *bounds,
                              double *par,
                              std::string* /*error*/,int *tile_width, int *tile_height)
{
    #ifdef DEBUG
    std::cout << "getFrameBounds ..." << std::endl;
    #endif

    int width = 0;
    int height = 0;
    _getImageSize(&width,&height,filename);
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

void ReadORAPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    #ifdef DEBUG
    std::cout << "onInputFileChanged ..." << std::endl;
    #endif

    assert(premult && components);

    int width = 0;
    int height = 0;
    _getImageSize(&width,&height,newFile);
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
    *premult = OFX::eImageOpaque;
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadORAPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void ReadORAPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"ora", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(50);
    #endif

    desc.setPluginDescription("Read ORA (OpenRaster) image format.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadORAPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadORAPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadORAPlugin* ret =  new ReadORAPlugin(handle);
    ret->restoreStateFromParameters();
    return ret;
}

void getReadORAPluginID(OFX::PluginFactoryArray &ids)
{
    static ReadORAPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
