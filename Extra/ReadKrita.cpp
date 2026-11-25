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
#include <cstdlib>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsImageEffect.h"
#include "lodepng.h"
#include "ReadZip.h"
#include "ReadHandles.h"

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

using namespace OFX;
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
    virtual void decode(const std::string& filename, OfxTime time, int view, const OfxRectI& renderWindow, const OfxPointD& renderScale, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, int view, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual bool guessParamsFromFilename(const std::string& filename, std::string *colorspace, OFX::PreMultiplicationEnum *filePremult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    void parseXML(xmlNode *node,int *width, int *height);
    void getImageSize(int *width, int *height, std::string filename);
    // the image size in maindoc.xml, from a parsed copy of it
    void getImageSizeFromXML(const std::string& xml, int *width, int *height);
};

ReadKritaPlugin::ReadKritaPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles, kIsMultiPlanar)
{
}

ReadKritaPlugin::~ReadKritaPlugin()
{
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
                // xmlGetProp returns NULL if the attribute is missing
                imgW = ArenaZip::parseInt(imgWchar);
                imgH = ArenaZip::parseInt(imgHchar);
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
    getImageSizeFromXML(ArenaZip::readEntry(filename, "maindoc.xml", ArenaZip::kMaxXMLSize), width, height);
}

void
ReadKritaPlugin::getImageSizeFromXML(const std::string& xml, int *width, int *height)
{
    if (!xml.empty()) {
        int imgW = 0;
        int imgH = 0;
        xmlDocPtr doc;
        doc = ArenaZip::parseXML(xml);
        xmlNode *root_element = NULL;
        root_element = xmlDocGetRootElement(doc);
        parseXML(root_element,&imgW,&imgH);
        xmlFreeDoc(doc);
        // no larger canvas than decodePNG() decodes: the host would allocate it (#214)
        if (imgW>0 && imgH>0 && std::uint64_t(imgW) * imgH * 4 <= ArenaZip::kMaxDecodedSize) {
            (*width)=imgW;
            (*height)=imgH;
        }
    }
}

void
ReadKritaPlugin::decode(const std::string& filename,
                      OfxTime /*time*/,
                      int /*view*/,
                      const OfxRectI& renderWindow,
                      const OfxPointD& renderScale,
                      float *pixelData,
                      const OfxRectI& bounds,
                      OFX::PixelComponentEnum pixelComponents,
                      int pixelComponentCount,
                      int rowBytes)
{
    assert(renderScale.x == 1. && renderScale.y == 1.);
    unused(renderScale);
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int width = 0;
    int height = 0;
    // the image size in maindoc.xml, read from the archive that the merged image is decoded from
    int docWidth = 0;
    int docHeight = 0;
    // lodepng's output, allocated with malloc; freed on every path, including the throws below and
    // in convertDepthAndComponents() (#132)
    Arena::MallocPtr<unsigned char> buffer;

    // NULL on failure (missing or corrupt file)
    const Arena::ZipPtr archive = ArenaZip::openArchive(filename);
    if (archive) {
        // the size in the archive is checked and capped (#128)
        const std::string imageData = ArenaZip::readEntry(archive.get(), "mergedimage.png", ArenaZip::kMaxImageSize);
        if (!imageData.empty()) {
            unsigned w = 0, h = 0;
            buffer = ArenaZip::decodePNG(imageData, &w, &h); // capped (#159)
            if (buffer) {
                width = (int)w;
                height = (int)h;
            }
        }
        if (buffer) {
            getImageSizeFromXML(ArenaZip::readEntry(archive.get(), "maindoc.xml", ArenaZip::kMaxXMLSize), &docWidth, &docHeight);
        }
    }

    // The whole merged image is decoded, and only the part of renderWindow that it covers is
    // copied: a host may ask for a render window smaller than the image, even without tiles
    // (#105).
    if (!buffer || width<=0 || height<=0) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    // The merged image is placed in the image of maindoc.xml (the frame bounds, (0,0)-(W,H)),
    // read from the same open archive (#108): no second zip_open, and the size is the one of the
    // file that the PNG comes from. Krita's y goes down from the top, OFX's goes up from the
    // bottom, so the PNG is anchored at the top-left corner: a PNG of another size than the image
    // is clamped, with transparent black around it (it used to fail with "Unable to read image").
    if (docHeight <= 0) {
        docHeight = height; // no size in maindoc.xml: the PNG is the image
    }
    OfxRectI image;
    image.x1 = 0;
    image.x2 = width;
    image.y2 = docHeight;
    image.y1 = docHeight - height;

    // buffer (lodepng's RGBA, 8 bits, packed, top-down) covers image: copy renderWindow as ReadSVG.cpp does
    OfxRectI copied;
    if (clipToFile(renderWindow, image, pixelData, bounds, pixelComponentCount, OFX::eBitDepthFloat, rowBytes, &copied)) {
        // GenericReader's conversion reads the top-down buffer directly (no transposed copy), with the
        // formula of the copy that it replaces (x * (1.f / 255)), so the output is the same
        convertDepthAndComponents(buffer.get(), copied, renderScale, image, OFX::ePixelComponentRGBA, OFX::eBitDepthUByte, width * 4,
                                  pixelData, bounds, pixelComponents, rowBytes);
    }
}

bool ReadKritaPlugin::getFrameBounds(const std::string& filename,
                                     OfxTime /*time*/,
                                     int /*view*/,
                                     OfxRectI *bounds,
                                     OfxRectI* format,
                                     double *par,
                                     std::string* error,int *tile_width, int *tile_height)
{
    int width = 0;
    int height = 0;
    getImageSize(&width,&height,filename);
    if (width <= 0 || height <= 0) {
        // a missing or corrupt file, or no image size in maindoc.xml: report it, instead of
        // returning true with the bounds unset, from which GenericReader made a garbage RoD (#129)
        if (error) {
            *error = "Unable to read image";
        }
        return false;
    }
    bounds->x1 = 0;
    bounds->x2 = width;
    bounds->y1 = 0;
    bounds->y2 = height;
    *format = *bounds;
    *par = 1.0;
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
    // libxml2 before 2.11 must be initialized on the main thread before it is used on others (#160)
    xmlInitParser();
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
