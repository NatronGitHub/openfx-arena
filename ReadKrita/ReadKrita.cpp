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
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsImageEffect.h"
#include "lodepng.h"

#define kPluginName "ReadKrita"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "fr.inria.openfx.ReadKrita"
#define kPluginVersionMajor 2
#define kPluginVersionMinor 0
#define kPluginEvaluation 50

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsXY false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar false

using namespace OFX::IO;

#ifdef OFX_IO_USING_OCIO
namespace OCIO = OCIO_NAMESPACE;
#endif

OFXS_NAMESPACE_ANONYMOUS_ENTER

class ReadKritaPlugin : public GenericReaderPlugin
{
public:
    ReadKritaPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions);
    virtual ~ReadKritaPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual bool guessParamsFromFilename(const std::string& filename, std::string *colorspace, OFX::PreMultiplicationEnum *filePremult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    std::string extractXML(std::string kritaFile);
    void parseXML(xmlNode *node,int *width, int *height);
    void getImageSize(int *width, int *height, std::string filename);
};

ReadKritaPlugin::ReadKritaPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles, kIsMultiPlanar)
{
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
        char *xml = new char[xmlSt.size+1];
        zip_file *xmlFile = zip_fopen(kritaOpen,xmlName,0);
        err=zip_fread(xmlFile,xml,xmlSt.size);
        if (err!=-1) {
            zip_fclose(xmlFile);
            xml[xmlSt.size] = '\0';
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

void
ReadKritaPlugin::decode(const std::string& filename,
                      OfxTime /*time*/,
                      int /*view*/,
                      bool /*isPlayback*/,
                      const OfxRectI& renderWindow,
                      float *pixelData,
                      const OfxRectI& /*bounds*/,
                      OFX::PixelComponentEnum /*pixelComponents*/,
                      int pixelComponentCount,
                      int /*rowBytes*/)
{
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int width = 0;
    int height = 0;
    unsigned char* buffer = NULL;
    int renderWidth= renderWindow.x2 - renderWindow.x1;
    int renderHeight= renderWindow.y2 - renderWindow.y1;

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
                lodepng_decode32(&buffer,(unsigned int*)&width,(unsigned int*)&height,(unsigned char *)imageData,(size_t)imageSt.size);
            }
        }
        delete[] imageData;
    }
    zip_close(imageOpen);

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

bool ReadKritaPlugin::getFrameBounds(const std::string& filename,
                                     OfxTime /*time*/,
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

bool ReadKritaPlugin::guessParamsFromFilename(const std::string& /*newFile*/,
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

    int width = 0;
    int height = 0;
    getImageSize(&width,&height,filename);
    if (width==0 && height==0) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    *components = OFX::ePixelComponentRGBA;
    *filePremult = OFX::eImageUnPreMultiplied;

    return true;
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadKritaPluginFactory, {}, false);

void
ReadKritaPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("kra");
}

/** @brief The basic describe function, passed a plugin descriptor */
void ReadKritaPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, false);
    desc.setLabel(kPluginName);
    desc.setPluginDescription("Read Krita image format.");
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadKritaPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles, true);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "scene_linear");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadKritaPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadKritaPlugin* ret =  new ReadKritaPlugin(handle, _extensions);
    ret->restoreStateFromParams();
    return ret;
}

static ReadKritaPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
