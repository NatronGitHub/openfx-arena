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
#include <sstream>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#include "ofxsMultiThread.h"
// fast_mutex can be constructed statically, before the host's multithread
// suite is available.
#include "fast_mutex.h"
typedef tthread::fast_mutex Mutex;
typedef OFX::MultiThread::AutoMutexT<tthread::fast_mutex> AutoMutex;

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
#define kPluginRepoURL "https://raw.githubusercontent.com/NatronGitHub/clut/master"
#define kPluginRepoZIP "https://github.com/NatronGitHub/clut/archive/master.zip"
// Version 3 uses the sRGB transfer function to apply the CLUT to linear input (see HaldCLUTPlugin::render),
// and unpremultiplies the source before ImageMagick (see PackedImage::PackedSrc).
#define kPluginVersionMajor 3
#define kPluginVersionMinor 0

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

// Shared by all instances, since they download to the same preset directory.
static Mutex gDownloadMutex;

static int
currentProcessId()
{
#ifdef _WIN32
    return _getpid();
#else
    return (int)getpid();
#endif
}

bool
existsFile(const std::string &filename)
{
    struct stat st;
    return (stat(filename.c_str(), &st) == 0);
}

struct HaldCLUTPreset
{
    std::string title;
    std::string category;
    std::string file;
    std::string checksum;
};

// Returns the text content of an XML node, or an empty string if it has none
// (xmlNodeListGetString returns NULL for an empty element).
static std::string
nodeString(xmlDocPtr doc, xmlNodePtr cur)
{
    std::string ret;
    xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
    if (key) {
        ret = reinterpret_cast<const char*>(key);
        xmlFree(key);
    }
    return ret;
}

// Fields are looked up by element name, not by position. Returns false if the
// preset is unusable (a missing title, category or file).
static bool
parsePreset(xmlDocPtr doc, xmlNodePtr cur, HaldCLUTPreset* preset)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"title"))) {
            preset->title = nodeString(doc, cur);
        } else if ((!xmlStrcmp(cur->name, (const xmlChar *)"file"))) {
            preset->file = nodeString(doc, cur);
        } else if ((!xmlStrcmp(cur->name, (const xmlChar *)"checksum"))) {
            preset->checksum = nodeString(doc, cur);
        } else if ((!xmlStrcmp(cur->name, (const xmlChar *)"category"))) {
            preset->category = nodeString(doc, cur);
        }
        cur = cur->next;
    }
    return !preset->title.empty() && !preset->category.empty() && !preset->file.empty();
}

// Both describeInContext (choice options) and the instance (render) build their
// table with this function, so choice index i always maps to (*presets)[i].
// Invalid presets are skipped instead of being stored as empty entries.
void
parseXML(const std::string &filename, std::vector<HaldCLUTPreset>* presets)
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
            HaldCLUTPreset preset;
            if (parsePreset(doc, cur, &preset)) {
                presets->push_back(preset);
            }
        }
        cur = cur->next;
    }

    xmlFreeDoc(doc);
    // Do not call xmlCleanupParser() here: it frees libxml2's global state,
    // which the host and other plugins in the same process may still be using.
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
    // fixed is false for version 1, which used a 2.2 power function instead of the sRGB
    // transfer function and fed premultiplied pixels to ImageMagick: version 3 fixes both
    HaldCLUTPlugin(OfxImageEffectHandle handle, bool fixed)
        : MagickPluginHelper<kSupportsRenderScale>(handle, fixed)
        , _srgbTransfer(fixed)
        , _presets()
        , _preset(NULL)
        , _custom(NULL)
        , _path(NULL)
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
                        if (presetFound == presetSelected) {
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
            if (preset >= 0 && preset < (int)_presets.size()) {
                category = _presets[preset].category;
                filename = _presets[preset].file;
            }
            //checksum = _presets[preset].checksum;
            if (category.empty() || filename.empty() /*|| checksum.empty()*/) {
                setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read XML");
                OFX::throwSuiteStatusException(kOfxStatFailed);
            }
            url.append(kPluginRepoURL);
            url.append("/" + category + "/" + filename);
            std::string dir = presetPath(customPath);
            if (dir.empty()) {
                setPersistentMessage(OFX::Message::eMessageError, "", "Unable to find the home directory, please set the 'customPath' param");
                OFX::throwSuiteStatusException(kOfxStatFailed);
            }
            clut.append(dir + "/" + filename);
            if (url.empty() || clut.empty()) {
                setPersistentMessage(OFX::Message::eMessageError, "", "Missing XML data");
                OFX::throwSuiteStatusException(kOfxStatFailed);
            }
            if (!existsFile(clut)) {
                if (!getFile(url, clut, true)) {
                    setPersistentMessage(OFX::Message::eMessageError, "", "Unable to download LUT");
                    OFX::throwSuiteStatusException(kOfxStatFailed);
                }
            }
            presetFile = clut;
        } else {
            presetFile = custom;
        }

        // The plugin is eRenderFullySafe, so several render threads may run on
        // this instance: _lut (the cached CLUT, keyed by its comment) is only
        // read or modified with _lutMutex held, and rendering uses a reference
        // to it (Magick::Image copies are reference-counted and copy-on-write,
        // so a later update of _lut does not modify lut).
        Magick::Image lut;
        {
            AutoMutex lock(_lutMutex);
            if (_lut.comment() != presetFile) {
                try {
                    _lut.read(presetFile.c_str());
                } catch(Magick::Exception &exp) {
                    if (custom.empty()) {
                        _lut.comment("N/A");
                        if (!getFile(url, clut, false)) {
                            setPersistentMessage(OFX::Message::eMessageError, "", "Unable to download LUT");
                            OFX::throwSuiteStatusException(kOfxStatFailed);
                        } else {
                            // This read used to be outside the try block, so its
                            // exception escaped render without an error message.
                            try {
                                _lut.read(presetFile.c_str());
                            } catch(Magick::Exception &) {
                                _lut.comment("N/A");
                                setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read CLUT");
                                OFX::throwSuiteStatusException(kOfxStatFailed);
                            }
                        }
                    }
                }
                if (!_srgbTransfer) {
                    _lut.colorSpace(Magick::RGBColorspace);
                }
                _lut.comment(presetFile);
            }
            lut = _lut;
        }

        if (lut.rows() >= 512 && lut.rows() == lut.columns() && lut.comment() == presetFile) {
            if (_srgbTransfer) {
                // The input is linear, while Hald CLUT files map sRGB-encoded values to
                // sRGB-encoded values. Encode the input with the sRGB transfer function, clamp it
                // to the domain of the CLUT, apply the CLUT unchanged, and decode its output back
                // to linear. Version 1 encoded the input with a 2.2 power function but decoded the
                // (linearised) CLUT with the sRGB function, so even an identity CLUT changed the
                // image (e.g. it lifted the shadows).
                // tag as linear, pixels unchanged (renamed colorSpaceType in IM7)
#if MagickLibVersion >= 0x700
                image.colorSpaceType(Magick::RGBColorspace);
#else
                image.colorspaceType(Magick::RGBColorspace);
#endif
                image.colorSpace(Magick::sRGBColorspace);
                image.clamp();
                image.haldClut(lut);
                image.colorSpace(Magick::RGBColorspace);
            } else {
                image.gamma(2.2);
                image.haldClut(lut);
            }
        } else {
            setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read CLUT");
            OFX::throwSuiteStatusException(kOfxStatFailed);
        }

    }
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    // Download location to destination. If onlyIfMissing is true and destination
    // already exists (e.g. another render thread just downloaded it), nothing is
    // done.
    bool getFile(const std::string &location, const std::string &destination, bool onlyIfMissing)
    {
        // Serialise downloads: concurrent render threads (or instances) would
        // otherwise fetch and write the same file at the same time, and
        // curl_easy_init() may run the non-thread-safe curl_global_init().
        AutoMutex lock(gDownloadMutex);

        if (onlyIfMissing && existsFile(destination)) {
            return true;
        }

        // Write to a temporary file and rename it on success, so that a failed
        // or partial download never leaves a file at destination (existsFile()
        // would then treat it as a valid cached preset).
        std::stringstream tmpss;
        tmpss << destination << ".part." << currentProcessId();
        const std::string tmp = tmpss.str();

        CURL *curl = curl_easy_init();
        if (!curl) {
            return false;
        }
        FILE *fp = fopen(tmp.c_str(), "wb");
        if (!fp) {
            curl_easy_cleanup(curl);
            return false;
        }
        curl_easy_setopt(curl, CURLOPT_URL, location.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        // Verify the server certificate (libcurl's default, set explicitly).
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        // Treat HTTP errors (e.g. 404) as failures, instead of saving the error
        // page as the preset file.
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        bool ok = (fclose(fp) == 0) && (res == CURLE_OK);
        if (ok) {
#ifdef _WIN32
            // rename() does not overwrite an existing file on Windows.
            remove(destination.c_str());
#endif
            ok = (rename(tmp.c_str(), destination.c_str()) == 0);
        }
        if (!ok) {
            remove(tmp.c_str());
        }
        return ok && existsFile(destination);
    }
    std::string presetPath(std::string altpath)
    {
        std::string path;
        if (altpath.empty()) {
            // getenv() returns NULL if the variable is unset, and appending NULL
            // to a std::string is undefined behaviour. Return an empty path then.
#ifdef _WIN32
            const char* homeDrive = getenv("HOMEDRIVE");
            const char* homePath = getenv("HOMEPATH");
            if (homeDrive && homePath) {
                path.append(homeDrive);
                path.append(homePath);
                path.append("/.clut");
            }
#else
            const char* home = getenv("HOME");
            if (home) {
                path.append(home);
                path.append("/.clut");
            }
#endif
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
    bool _srgbTransfer;
    std::vector<HaldCLUTPreset> _presets;
    ChoiceParam *_preset;
    Magick::Image _lut; // protected by _lutMutex
    Mutex _lutMutex;
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
        AutoMutex lock(_lutMutex);
        _lut.comment("N/A");
    }
    clearPersistentMessage();
}

mDeclarePluginFactoryVersioned(HaldCLUTPluginFactory, {}, {});

template<unsigned int majorVersion>
void
HaldCLUTPluginFactory<majorVersion>::describe(ImageEffectDescriptor &desc)
{
    if (majorVersion < kPluginVersionMajor) {
        desc.setIsObsolete(true);
    }
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

template<unsigned int majorVersion>
void
HaldCLUTPluginFactory<majorVersion>::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (getImageEffectHostDescription()->isNatron);

    std::string filename;
    filename.append(desc.getPropertySet().propGetString(kOfxPluginPropFilePath, false));
    filename.append("/Contents/Resources/");
    filename.append(kPluginIdentifier);
    filename.append(".xml");
    std::vector<HaldCLUTPreset> presets;
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
        // One option per entry of presets (all have a non-empty title, see
        // parsePreset), so that option indices match the instance's _presets.
        for (size_t i = 0; i < presets.size(); i++) {
            std::string presetTitle = presets[i].title;
            {
                if (gHostIsNatron) {
                    std::string tmp = presetTitle;
                    std::string presetCategory = presets[i].category;
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

template<unsigned int majorVersion>
OFX::ImageEffect*
HaldCLUTPluginFactory<majorVersion>::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new HaldCLUTPlugin(handle, majorVersion >= 3);
}

static HaldCLUTPluginFactory<kPluginVersionMajor> p(kPluginIdentifier, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
// Version 1 is kept unchanged so that existing projects render as before.
static HaldCLUTPluginFactory<1> p1(kPluginIdentifier, 1);
mRegisterPluginFactoryInstance(p1)

OFXS_NAMESPACE_ANONYMOUS_EXIT
