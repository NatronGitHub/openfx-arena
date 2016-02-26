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
#include <ofxNatron.h>
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
#define kPluginEvaluation 50

// http://www.freedesktop.org/wiki/Specifications/OpenRaster/Draft/
#define OpenRasterVersion 0.0.5
// We don't support versions under 0.0.2 since merged image is mandatory,
// but Krita uses/expose version 0.0.1, but has merged image (0.0.1 has that as optional)
// GIMP don't even specify version, and don't have a merged image, so we asume they follow 0.0.1 without (some) optional features.
// So... don't check for version at this moment, just check for merged image, if not present, add first layer twice and move on...

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
    Magick::Image getLayer(std::string filename, std::string layerfile);
    void getLayersSpecs(xmlNode *node, std::vector<std::vector<std::string> > *layers);
    void setupImage(std::string filename, std::vector<Magick::Image> *images);
    std::vector<Magick::Image> _layers;
    std::string _filename;
    bool _hasPNG;
};

OpenRasterPlugin::OpenRasterPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles,
#ifdef OFX_EXTENSIONS_NUKE
(OFX::getImageEffectHostDescription() && OFX::getImageEffectHostDescription()->isMultiPlanar) ? kIsMultiPlanar : false
#else
false
#endif
)
,_hasPNG(false)
{
    Magick::InitializeMagick(NULL);

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("png") != std::string::npos)
        _hasPNG = true;
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
            if (_hasPNG) {
                int width = 0;
                int height = 0;
                getImageSize(&width,&height,filename);
                Magick::Image image(getLayer(filename,""));
                bool hasMerged = false;
                if ((int)image.columns()==width && (int)image.rows()==height) {
                    hasMerged=true;
                    images->push_back(image); // add merged image
                }
                if (!layersInfo.empty()) {
                    std::reverse(layersInfo.begin(),layersInfo.end());
                    for (int i = 0; i < (int)layersInfo.size(); i++) {
                        Magick::Image layer(getLayer(filename,layersInfo[i][6]));
                        if (layer.format()=="Portable Network Graphics" && layer.columns()>0 && layer.rows()>0) {
                            int xOffset = atoi(layersInfo[i][4].c_str());
                            int yOffset = atoi(layersInfo[i][5].c_str());
                            layer.label(layersInfo[i][0]);
                            std::string comment = layersInfo[i][3] + " " + layersInfo[i][1] + " " + layersInfo[i][2]; // Not currently used, but may be needed in the future
                            layer.comment(comment);
                            layer.page().xOff(xOffset);
                            layer.page().yOff(yOffset);
                            images->push_back(layer);
                            if (!hasMerged && i==0) // GIMP workaround (or any other app that don't follow v0.0.2+)
                                images->push_back(layer);
                        }
                    }
                }
            }
        }
    }
}

Magick::Image
OpenRasterPlugin::getLayer(std::string filename, std::string layer)
{
    if (layer.empty())
        layer = "mergedimage.png"; // asume we want the merged image if no layer has been specified
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
                if (_hasPNG) {
                    Magick::Blob blob(layerData,layerSt.size);
                    Magick::Image tmp(blob);
                    if (tmp.format()=="Portable Network Graphics")
                        image=tmp;
                }
            }
        }
        delete[] layerData;
    }
    zip_close(layerOpen);
    return image;
}

void
OpenRasterPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    assert(isMultiPlanar());
    clipComponents.addClipComponents(*_outputClip, getOutputComponents());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (_layers.size()>0 && gHostIsNatron) {
        for (int i = 1; i < (int)_layers.size(); i++) {
            std::ostringstream layerName;
            layerName << _layers[i].label();
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
OpenRasterPlugin::decodePlane(const std::string& /*filename*/, OfxTime /*time*/, int /*view*/, bool /*isPlayback*/, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds,
                                 OFX::PixelComponentEnum /*pixelComponents*/, int /*pixelComponentCount*/, const std::string& rawComponents, int /*rowBytes*/)
{
    int layer = 0;
    int offsetX = 0;
    int offsetY = 0;

    if (_layers.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Empty and/or corrupt image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    if (!_hasPNG) {
        setPersistentMessage(OFX::Message::eMessageError, "", "PNG support missing!");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // Get multiplane layer
    std::string layerName;
    std::vector<std::string> layerChannels = OFX::mapPixelComponentCustomToLayerChannels(rawComponents);
    int numChannels = layerChannels.size();
    if (numChannels==5) // layer+R+G+B+A
        layerName=layerChannels[0];
    if (!layerName.empty()) {
        for (int i = 1; i < (int)_layers.size(); i++) {
            bool foundLayer = false;
            std::ostringstream nonameLayer;
            nonameLayer << "Image Layer #" << i; // if layer name is empty
            if (_layers[i].label()==layerName)
                foundLayer = true;
            if (nonameLayer.str()==layerName && !foundLayer)
                foundLayer = true;
            if (foundLayer) {
                offsetX = _layers[i].page().xOff();
                offsetY = _layers[i].page().yOff();
                layer = i;
                break;
            }
        }
    }
    else {
        offsetX = 0;
        offsetY = 0;
        layer = 0;
    }

    Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
    if ((int)_layers[layer].columns()==bounds.x2 && (int)_layers[layer].rows()==bounds.y2) {
        container.composite(_layers[layer],offsetX,offsetY,Magick::OverCompositeOp);
        container.flip();
        container.write(0,0,renderWindow.x2 - renderWindow.x1,renderWindow.y2 - renderWindow.y1,"RGBA",Magick::FloatPixel,pixelData);
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to load image");
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

void
OpenRasterPlugin::restoreState(const std::string& filename)
{
    _layers.clear();
    if (!filename.empty()) {
            setupImage(filename,&_layers);
    }
    if (!_layers.empty()) {
        if (_layers[0].columns()>0 && _layers[0].rows()>0)
            _filename = filename;
    }
    else {
        _layers.clear();
        setPersistentMessage(OFX::Message::eMessageError, "", "Empty and/or corrupt image");
    }
}

void OpenRasterPlugin::onInputFileChanged(const std::string& newFile,
                                  bool setColorSpace,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    assert(premult && components);
    if (newFile!=_filename)
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
