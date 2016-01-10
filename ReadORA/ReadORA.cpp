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
#define kPluginVersionMinor 6

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

std::string _extractXML(std::string oraFile)
{
    #ifdef DEBUG
    std::cout << "_extractXML ..." << std::endl;
    #endif

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
    #ifdef DEBUG
    std::cout << "_getImageSize ..." << std::endl;
    #endif

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

    void setupImage(std::string filename,std::vector<Magick::Image> *images);
    void getLayersInfo(xmlNode *node,std::vector<std::vector<std::string> > *layers);
    Magick::Image getLayer(std::string filename, std::string layerfile);
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
ReadORAPlugin::getLayersInfo(xmlNode *node,std::vector<std::vector<std::string> > *layers)
{
    #ifdef DEBUG
    std::cout << "getLayersInfo ..." << std::endl;
    #endif

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
                if (!layerFile.empty()) {
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
        getLayersInfo(cur_node->children,layers);
    }
}

Magick::Image
ReadORAPlugin::getLayer(std::string filename, std::string layer)
{
    #ifdef DEBUG
    std::cout << "getLayer ..." << std::endl;
    #endif

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
ReadORAPlugin::setupImage(std::string filename,std::vector<Magick::Image> *images)
{
    #ifdef DEBUG
    std::cout << "setupImage ..." << std::endl;
    #endif

    if (!filename.empty()) {
        std::string xml = _extractXML(filename);
        if (!xml.empty()) {
            std::vector<std::vector<std::string> > layersInfo;
            xmlDocPtr doc;
            doc = xmlParseDoc((const xmlChar *)xml.c_str());
            xmlNode *root_element = NULL;
            root_element = xmlDocGetRootElement(doc);
            getLayersInfo(root_element,&layersInfo);
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
                        std::string comment = layersInfo[i][3] + " " + layersInfo[i][1] + " " + layersInfo[i][2]; // comp+opacity+visible
                        layer.comment(comment);
                        layer.page().xOff(xOffset);
                        layer.page().yOff(yOffset);
                        images->push_back(layer);
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

    /* TODO:
     * multiplane (no point before the rest is done)
     * if first layer is hidden (and we dont include it) it breaks the comp (and get NaN in Natron)
     * opacity don't work 100% yet
     * missing two compositeop
     * colors don't match 100% (in viewer at least)
    */

    int width = 0;
    int height = 0;
    if (!filename.empty())
        _getImageSize(&width,&height,filename);
    if (width==bounds.x2 && height==bounds.y2) {
        std::vector<Magick::Image> _image;
        setupImage(filename,&_image);
        Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,1)"));
        Magick::Image image(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
        if (!_image.empty()) {
            for (int i = 0; i < (int)_image.size(); i++) {
                #ifdef DEBUG
                std::cout << _image[i].label() << " " << _image[i].comment() << std::endl;
                #endif
                std::string composite, opacity, visibility;
                std::istringstream stream(_image[i].comment());
                std::string result;
                int count = 0;
                while ( std::getline(stream,result,' ')) {
                    if (count==0 && !result.empty())
                        composite=result;
                    if (count==1 && !result.empty())
                        opacity=result;
                    if (count==2 && !result.empty())
                        visibility=result;
                    count++;
                }
                int compositeOp = 0;
                if (composite=="svg:add")
                    compositeOp=1;
                else if (composite=="svg:color-burn")
                    compositeOp=2;
                else if (composite=="svg:color")
                    compositeOp=3;
                else if (composite=="svg:color-dodge")
                    compositeOp=4;
                else if (composite=="svg:darken")
                    compositeOp=5;
                else if (composite=="krita:erase")
                    compositeOp=6;
                else if (composite=="svg:lighten")
                    compositeOp=7;
                else if (composite=="svg:luminosity")
                    compositeOp=8;
                else if (composite=="svg:multiply")
                    compositeOp=9;
                else if (composite=="svg:overlay")
                    compositeOp=10;
                else if (composite=="svg:saturation")
                    compositeOp=11;
                else if (composite=="svg:screen")
                    compositeOp=12;
                else if (composite=="svg:soft-light")
                    compositeOp=13;

                double opacityVal = atof(opacity.c_str());
                double grayVal;
                grayVal=opacityVal*100;
                if (visibility=="1") {
                    #ifdef DEBUG
                    std::cout << "adding layer " << _image[i].label() << std::endl;
                    #endif
                    if (opacityVal<1) { //semi-broken
                        std::ostringstream grayColor;
                        grayColor<< "gray";
                        grayColor<< (int)grayVal;
                        Magick::Image mask(_image[i]);
                        mask.channel(Magick::OpacityChannel);
                        mask.colorFuzz(MaxRGB*opacityVal);
                        try {
                            mask.opaque(Magick::Color("black"),Magick::Color(grayColor.str()));
                        }
                        catch(Magick::Warning &warning) {
                            #ifdef DEBUG
                            std::cout << warning.what() << std::endl;
                            #endif
                            mask.opaque(Magick::Color("black"),Magick::Color(grayColor.str()));
                        }
                        mask.negate();
                        _image[i].composite(mask,0,0,Magick::CopyOpacityCompositeOp);
                    }
                    switch (compositeOp) { // http://www.imagemagick.org/Usage/compose/
                    case 1:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::AddCompositeOp);
                        break;
                    case 2:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::ColorBurnCompositeOp);
                        break;
                    case 3: // can't find
                        setPersistentMessage(OFX::Message::eMessageError, "", "svg:color compositeop no supported yet");
                        OFX::throwSuiteStatusException(kOfxStatErrFormat);
                        //image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::OverCompositeOp);
                        break;
                    case 4:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::ColorDodgeCompositeOp);
                        break;
                    case 5:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::DarkenCompositeOp);
                        break;
                    case 6: // don't know yet
                        setPersistentMessage(OFX::Message::eMessageError, "", "krita:erase compositeop not supported yet");
                        OFX::throwSuiteStatusException(kOfxStatErrFormat);
                        //image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::OverCompositeOp);
                        break;
                    case 7:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::LightenCompositeOp);
                        break;
                    case 8:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::LuminizeCompositeOp);
                        break;
                    case 9:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::MultiplyCompositeOp);
                        break;
                    case 10:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::OverlayCompositeOp);
                        break;
                    case 11:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::SaturateCompositeOp);
                        break;
                    case 12:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::ScreenCompositeOp);
                        break;
                    case 13:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::SoftLightCompositeOp);
                        break;
                    default:
                        image.composite(_image[i],_image[i].page().xOff(),_image[i].page().yOff(),Magick::OverCompositeOp);
                        break;
                    }
                }
                else {
                    #ifdef DEBUG
                    std::cout << "skipping layer (hidden) " << _image[i].label() << std::endl;
                    #endif
                }
            }
        }

        // return image
        // image.colorSpace(Magick::RGBColorspace);
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

    desc.setPluginDescription("Read Open Raster image format.");
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
