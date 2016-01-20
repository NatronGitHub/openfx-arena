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

#define kPluginName "ReadKritaOFX"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadKrita"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

class ReadKritaPlugin : public GenericReaderPlugin
{
public:
    ReadKritaPlugin(OfxImageEffectHandle handle);
    virtual ~ReadKritaPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, bool setColorSpace, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    std::string extractXML(std::string kritaFile);
    void parseXML(xmlNode *node,int *width, int *height);
    void getImageSize(int *width, int *height, std::string filename);
    Magick::Image getImage(std::string filename);
    bool _hasPNG;
};

ReadKritaPlugin::ReadKritaPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
,_hasPNG(false)
{
    Magick::InitializeMagick(NULL);

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("png") != std::string::npos)
        _hasPNG = true;
}

ReadKritaPlugin::~ReadKritaPlugin()
{
}

std::string
ReadKritaPlugin::extractXML(std::string kritaFile)
{
    std::string output;
    int err = 0;
    zip *kritaOpen = zip_open(kritaFile.c_str(), 0, &err);
    const char *xmlName = "maindoc.xml";
    struct zip_stat xmlSt;
    zip_stat_init(&xmlSt);
    err=zip_stat(kritaOpen,xmlName,0,&xmlSt);
    if (err!=-1) {
        char *xml = new char[xmlSt.size];
        zip_file *xmlFile = zip_fopen(kritaOpen,xmlName,0);
        err=zip_fread(xmlFile,xml,xmlSt.size);
        if (err!=-1) {
            zip_fclose(xmlFile);
            xml[xmlSt.size-1] = '\0';
            output=xml;
        }
        delete[] xml;
    }
    zip_close(kritaOpen);
    return output;
}

void
ReadKritaPlugin::parseXML(xmlNode *node,int *width, int *height)
{
    bool endLoop = false;
    xmlNode *cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"IMAGE"))) {
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
                if (imgW>0&&imgH>0) {
                    (*width)=imgW;
                    (*height)=imgH;
                    endLoop=true;
                }
            }
        }
        if (!endLoop)
            parseXML(cur_node->children,width,height);
    }
}

void
ReadKritaPlugin::getImageSize(int *width, int *height, std::string filename)
{
    std::string xml = extractXML(filename);
    if (!xml.empty()) {
        int imgW = 0;
        int imgH = 0;
        xmlDocPtr doc;
        doc = xmlParseDoc((const xmlChar*)xml.c_str());
        xmlNode *root_element = NULL;
        root_element = xmlDocGetRootElement(doc);
        parseXML(root_element,&imgW,&imgH);
        xmlFreeDoc(doc);
        if (imgW>0 && imgH>0) {
            (*width)=imgW;
            (*height)=imgH;
        }
    }
}

Magick::Image
ReadKritaPlugin::getImage(std::string filename)
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
            if ((imageData!=NULL)&&(imageSt.size>0)) {
                if (_hasPNG) {
                    Magick::Blob blob(imageData,imageSt.size);
                    Magick::Image tmp(blob);
                    if (tmp.format()=="Portable Network Graphics")
                        image=tmp;
                }
                else
                    setPersistentMessage(OFX::Message::eMessageError, "", "PNG support missing!");
            }
        }
        delete[] imageData;
    }
    zip_close(imageOpen);
    return image;
}

void
ReadKritaPlugin::decode(const std::string& filename,
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
    if (!_hasPNG) {
        setPersistentMessage(OFX::Message::eMessageError, "", "PNG support missing!");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

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

bool ReadKritaPlugin::getFrameBounds(const std::string& filename,
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

void ReadKritaPlugin::onInputFileChanged(const std::string& newFile,
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

mDeclareReaderPluginFactory(ReadKritaPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void ReadKritaPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"kra", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(50);
    #endif

    desc.setPluginDescription("Read Krita image format.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadKritaPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadKritaPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadKritaPlugin* ret =  new ReadKritaPlugin(handle);
    ret->restoreStateFromParameters();
    return ret;
}

static ReadKritaPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
