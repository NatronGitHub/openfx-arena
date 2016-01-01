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
#define kPluginVersionMinor 1

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

// TODO error checking
std::string _extractXML(std::string oraFile)
{
    std::string output;

    int err = 0;
    zip *oraOpen = zip_open(oraFile.c_str(), 0, &err);

    const char *xmlName = "stack.xml";
    struct zip_stat xmlSt;
    zip_stat_init(&xmlSt);
    zip_stat(oraOpen, xmlName, 0, &xmlSt);

    char *xml = new char[xmlSt.size];

    zip_file *xmlFile = zip_fopen(oraOpen, "stack.xml", 0);
    zip_fread(xmlFile, xml, xmlSt.size);
    zip_fclose(xmlFile);
    zip_close(oraOpen);

    xml[xmlSt.size] = '\0';

    output = xml;

    return output;
}

// TODO
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

    Magick::Image image;
    int width = 0;
    int height = 0;
    if (!filename.empty())
        _getImageSize(&width,&height,filename);
    if (width>0 && height>0) {
        Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,1)"));
        //container.composite(image,0,0,Magick::OverCompositeOp);
        //container.composite(image,0,0,Magick::CopyOpacityCompositeOp);
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
