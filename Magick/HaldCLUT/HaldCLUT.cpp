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

#include "MagickPlugin.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <curl/curl.h>

#include <stdio.h>
#include <string>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "HaldCLUT"
#define kPluginGrouping "Extra/Color"
#define kPluginIdentifier "net.fxarena.openfx.HaldCLUT"
#define kPluginDescription \
    "A Hald CLUT is an image that has a specific color pattern on it. " \
    "In this pattern all colors in the color space are represented. " \
    "An application that uses the HALD CLUT image to color correct an image, " \
    "takes a source image color and looks it up in the color pattern of the HALD CLUT, "\
    "and the color it finds in that place is the corrected color that should replace the source color in the destination image. " \
    "If the color doesn't exist in the CLUT, one can look up several colors and interpolate between them." \
    "\n\n" \
    "http://www.quelsolaar.com/technology/clut.html" \
    "\n\n" \
    "HaldCLUT has a large collection (300+) of CLUT presets divided over several categories:\n\n" \
    "* Black and White (by Pat David)\n" \
    "* Instant Consumer (by Pat David)\n" \
    "* Instant Pro (by Pat David)\n" \
    "* Negative Color (by Pat David)\n" \
    "* Negative New (by Pat David)\n" \
    "* Negative Old (by Pat David)\n" \
    "* PictureFX (by Marc Roovers)\n" \
    "* Print Films (by Juan Melara)\n" \
    "* Slide (Color) (by Pat David)\n" \
    "* Various" \
    "\n\n" \
    "The CLUT presets is licenced under the Creative Commons Attribution-ShareAlike license." \
    "\n\n" \
    "CLUT presets are downloaded on-demand if missing from disk, an active internet connection is needed during that time." \
    "\n\n" \
    "Trademarked names which may appear are there for informational purposes only. " \
    "They serve only to inform the user which film stock the given HaldCLUT image is designed to approximate. " \
    "As there is no way to convey this information other than by using the trademarked name, we believe this constitutes fair use. " \
    "Neither the publisher nor the authors are affiliated with or endorsed by the companies that own the trademarks."
#define kPluginRepoURL "https://raw.githubusercontent.com/olear/clut/master"
#define kPluginRepoZIP "https://github.com/olear/clut/archive/master.zip"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 1

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamPreset "look"
#define kParamPresetLabel "Look"
#define kParamPresetHint "Select CLUT preset."
#define kParamPresetDefault 0

#define kParamCustom "custom"
#define kParamCustomLabel "Custom CLUT File"
#define kParamCustomHint "Add a custom CLUT PNG file."

#define kParamCustomPath "customPath"
#define kParamCustomPathLabel "Custom Preset Path"
#define kParamCustomPathHint "Add a custom path where the presets are located (or will be downloaded)."

static bool gHostIsNatron = false;

bool
existsFile(const std::string &filename)
{
    struct stat st;
    return (stat(filename.c_str(), &st) == 0);
}

std::vector<std::string>
parsePreset(xmlDocPtr doc, xmlNodePtr cur)
{
    cur = cur->xmlChildrenNode;
    xmlChar *key;
    std::vector<std::string> preset;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"title"))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            preset.push_back((reinterpret_cast<char*>(key)));
            xmlFree(key);
        }
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"file"))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            preset.push_back((reinterpret_cast<char*>(key)));
            xmlFree(key);
        }
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"checksum"))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            preset.push_back((reinterpret_cast<char*>(key)));
            xmlFree(key);
        }
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"category"))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            preset.push_back((reinterpret_cast<char*>(key)));
            xmlFree(key);
        }
        cur = cur->next;
    }
    if (preset.size() != 4) {
        preset.clear();
    }
    return preset;
}

void
parseXML(const std::string &filename, std::vector<std::vector<std::string> >* presets)
{
    xmlDocPtr doc;
    xmlNodePtr cur;

    if (!existsFile(filename)) {
        return;
    }

    doc = xmlParseFile(filename.c_str());

    if (doc == NULL ) {
        return;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        xmlFreeDoc(doc);
        return;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *)"looks")) {
        xmlFreeDoc(doc);
        return;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"preset"))) {
            presets->push_back(parsePreset(doc, cur));
        }
        cur = cur->next;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();
}

size_t
curlWriteData(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

class HaldCLUTPlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    HaldCLUTPlugin(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
        , _presets(0)
        , _preset(0)
        , _custom(0)
        , _path(0)
    {
        _preset = fetchChoiceParam(kParamPreset);
        _custom = fetchStringParam(kParamCustom);
        _path = fetchStringParam(kParamCustomPath);
        assert(_preset && _custom && _path);

        std::string xml;
        xml.append(getPropertySet().propGetString(kOfxPluginPropFilePath, false));
        xml.append("/Contents/Resources/");
        xml.append(kPluginIdentifier);
        xml.append(".xml");
        parseXML(xml, &_presets);

        if (_presets.size() > 0) {
            std::string presetSelected;
            int pid;
            int opts = _preset->getNOptions();
            _preset->getValue(pid);
            _preset->getOption(pid,presetSelected);
            if (!presetSelected.empty()) {
                for(int x = 0; x < opts; x++) {
                    std::string presetFound;
                    _preset->getOption(x, presetFound);
                    if (!presetFound.empty()) {
                        if (std::strcmp(presetFound.c_str(), presetSelected.c_str())==0) {
                             _preset->setValue(x);
                            break;
                        }
                     }
                }
            }
        }

        _lut = Magick::Image(Magick::Geometry(512, 512), Magick::Color("rgb(0,0,0)"));
        _lut.comment("N/A");

        std::string currPath;
        _path->getValue(currPath);
        if (currPath.empty()) {
            currPath = presetPath("");
        }
        std::stringstream msg;
        msg << "This plugin will download presets if they are not found on disk. The presets are stored in the " << currPath << " directory, you can override this location with the 'customPath' param. You can also download the presets manually from " << kPluginRepoZIP << ", extract and copy the preset files to the " << currPath << " directory.";
        setPersistentMessage(OFX::Message::eMessageMessage, "", msg.str());
    }

    virtual void render(const OFX::RenderArguments &args, Magick::Image &image) OVERRIDE FINAL
    {
        int preset = 0;
        std::string custom, url, clut, category, filename/*, checksum*/, customPath;
        _preset->getValueAtTime(args.time, preset);
        _custom->getValueAtTime(args.time, custom);
        _path->getValueAtTime(args.time, customPath);

        std::string presetFile;

        if (custom.empty()) {
            if (_presets.size() > 0) {
                category = _presets[preset][1];
                filename = _presets[preset][2];
            }
            //checksum = _presets[preset][3];
            if (category.empty() || filename.empty() /*|| checksum.empty()*/) {
                setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read XML");
                OFX::throwSuiteStatusException(kOfxStatFailed);
            }
            url.append(kPluginRepoURL);
            url.append("/" + category + "/" + filename);
            clut.append(presetPath(customPath) + "/" + filename);
            if (url.empty() || clut.empty()) {
                setPersistentMessage(OFX::Message::eMessageError, "", "Missing XML data");
                OFX::throwSuiteStatusException(kOfxStatFailed);
            }
            if (!existsFile(clut)) {
                if (!getFile(url, clut)) {
                    setPersistentMessage(OFX::Message::eMessageError, "", "Unable to download LUT");
                    OFX::throwSuiteStatusException(kOfxStatFailed);
                }
            }
            presetFile = clut;
        } else {
            presetFile = custom;
        }

        if (_lut.comment() != presetFile) {
            try {
                _lut.read(presetFile.c_str());
            } catch(Magick::Exception &exp) {
                if (custom.empty()) {
                    _lut.comment("N/A");
                    if (!getFile(url, clut)) {
                        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to download LUT");
                        OFX::throwSuiteStatusException(kOfxStatFailed);
                    } else {
                        _lut.read(presetFile.c_str());
                    }
                }
            }
            _lut.colorSpace(Magick::RGBColorspace);
            _lut.comment(presetFile);
        }

        if (_lut.rows() >= 512 && _lut.rows() == _lut.columns() && _lut.comment() == presetFile) {
            image.gamma(2.2);
            image.haldClut(_lut);
        } else {
            setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read CLUT");
            OFX::throwSuiteStatusException(kOfxStatFailed);
        }

    }
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    bool getFile(const std::string location, const std::string &destination)
    {
        CURL *curl;
        FILE *fp;
        CURLcode res;
        curl = curl_easy_init();
        if (curl) {
            fp = fopen(destination.c_str(),"wb");
            curl_easy_setopt(curl, CURLOPT_URL, location.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteData);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);
        }
        if (existsFile(destination) && res == CURLE_OK) {
            return true;
        } else {
            return false;
        }
    }
    std::string presetPath(std::string altpath)
    {
        std::string path;
        if (altpath.empty()) {
#ifdef _WIN32
            path.append(getenv("HOMEDRIVE"));
            path.append(getenv("HOMEPATH"));
#else
            path.append(getenv("HOME"));
#endif
            path.append("/.clut");
        }
        if (!path.empty()) {
            if (!existsFile(path)) {
#ifdef _WIN32
                mkdir(path.c_str());
#else
                mkdir(path.c_str(), 0750);
#endif
            }
        }
        if (!altpath.empty()) {
            path = altpath;
        }
        return path;
    }
private:
    std::vector<std::vector<std::string> > _presets;
    ChoiceParam *_preset;
    Magick::Image _lut;
    StringParam *_custom;
    StringParam *_path;
};

void
HaldCLUTPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (!_renderscale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    if (paramName == kParamCustom) {
        _lut.comment("N/A");
    }
    clearPersistentMessage();
}

mDeclarePluginFactory(HaldCLUTPluginFactory, {}, {});

void
HaldCLUTPluginFactory::describe(ImageEffectDescriptor &desc)
{
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);
    desc.addSupportedBitDepth(eBitDepthFloat);
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
    desc.setHostMaskingEnabled(kHostMasking);
    desc.setHostMixingEnabled(kHostMixing);
}

void
HaldCLUTPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (getImageEffectHostDescription()->isNatron);

    std::string filename;
    filename.append(desc.getPropertySet().propGetString(kOfxPluginPropFilePath, false));
    filename.append("/Contents/Resources/");
    filename.append(kPluginIdentifier);
    filename.append(".xml");
    std::vector<std::vector<std::string> > presets;
    parseXML(filename, &presets);

    PageParamDescriptor *page = HaldCLUTPlugin::describeInContextBegin(desc, context);
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamPreset);
        param->setLabel(kParamPresetLabel);
        param->setHint(kParamPresetHint);
        param->setDefault(kParamPresetDefault);
        if (gHostIsNatron) {
            param->setCascading(getImageEffectHostDescription()->supportsCascadingChoices);
        }
        for (size_t i = 0; i < presets.size(); i++) {
            std::string presetTitle = presets[i][0];
            if (!presetTitle.empty()) {
                if (gHostIsNatron) {
                    std::string tmp = presetTitle;
                    std::string presetCategory = presets[i][1];
                    if (presetCategory == "bw") {
                        presetCategory = "Black and White";
                    } else if (presetCategory == "colorslide") {
                        presetCategory = "Slide (Color)";
                    } else if (presetCategory == "instant_consumer") {
                        presetCategory = "Instant Consumer";
                    } else if (presetCategory == "instant_pro") {
                        presetCategory = "Instant Pro";
                    } else if (presetCategory == "negative_color") {
                        presetCategory = "Negative Color";
                    } else if (presetCategory == "negative_new") {
                        presetCategory = "Negative New";
                    } else if (presetCategory == "negative_old") {
                        presetCategory = "Negative Old";
                    } else if (presetCategory == "picturefx") {
                        presetCategory = "PictureFX";
                    } else if (presetCategory == "print") {
                        presetCategory = "Print Files";
                    } else if (presetCategory == "various") {
                        presetCategory = "Various";
                    }
                    presetTitle=presetCategory;
                    presetTitle.append("/" + tmp);
                }
                param->appendOption(presetTitle);
            }
        }
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamCustom);
        param->setLabel(kParamCustomLabel);
        param->setHint(kParamCustomHint);
        param->setStringType(eStringTypeFilePath);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamCustomPath);
        param->setLabel(kParamCustomPathLabel);
        param->setHint(kParamCustomPathHint);
        param->setStringType(eStringTypeDirectoryPath);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    HaldCLUTPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
HaldCLUTPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new HaldCLUTPlugin(handle);
}

static HaldCLUTPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
