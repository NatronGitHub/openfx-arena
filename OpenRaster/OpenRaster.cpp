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
    //void getImageSizeFromXML(xmlNode *node,int *width, int *height);
    void getImageSize(int *width, int *height, std::string filename);
    //Magick::Image getImage(std::string filename);
    std::vector<Magick::Image> image;
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
#ifdef DEBUG
    std::cout << output << std::endl;
#endif
    zip_close(fileOpen);
    return output;
}

/*void
OpenRasterPlugin::getImageSizeFromXML(xmlNode *node, int *width, int *height)
{
    bool endLoop = false;
    xmlNode *cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(cur_node->name,(const xmlChar *)"IMAGE"))) {
                int imgW = 0;
                int imgH = 0;
                xmlChar *imgWchar;
                xmlChar *imgHchar;
                imgWchar = xmlGetProp(cur_node,(const xmlChar*)"width");
                imgHchar = xmlGetProp(cur_node,(const xmlChar*)"height");
                imgW = atoi((const char*)imgWchar);
                imgH = atoi((const char*)imgHchar);
                xmlFree(imgWchar);
                xmlFree(imgHchar);
                if (imgW>0 && imgH>0) {
                    (*width)=imgW;
                    (*height)=imgH;
                    endLoop=true;
                }
            }
        }
        if (!endLoop)
            getImageSizeFromXML(cur_node->children,width,height);
    }
}*/

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

/*Magick::Image
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
}*/

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
    Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
    //Magick::Image image(getImage(filename));
    //if ((int)image.columns()==bounds.x2 && (int)image.rows()==bounds.y2) {
        //container.composite(image,0,0,Magick::OverCompositeOp);
        container.flip();
        container.write(0,0,renderWindow.x2 - renderWindow.x1,renderWindow.y2 - renderWindow.y1,"RGBA",Magick::FloatPixel,pixelData);
    /*}
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }*/
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
