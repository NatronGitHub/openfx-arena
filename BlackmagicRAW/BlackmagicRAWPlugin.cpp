/*
###################################################################################
#
# BlackmagicRAWOFX
#
# Copyright (C) 2020 Ole-André Rodlie <ole.andre.rodlie@gmail.com>
#
# BlackmagicRAWOFX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as published
# by the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# BlackmagicRAWOFX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
#
###################################################################################
*/

#include "BlackmagicRAWHandler.h"
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsImageEffect.h"

#define kPluginName "BlackmagicRAWOFX"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.sf.openfx.BlackmagicRAW"

// keep version in sync with SDK
#define kPluginVersionMajor 1
#define kPluginVersionMinor 8
#define kPluginEvaluation 0

#define kPluginDescription \
    "Blackmagic RAW is a modern, high performance, professional RAW codec that is open, cross platform and free.\n\n" \
    "Supported cameras:\n\n" \
    " - Blackmagic Design Pocket Cinema Camera 4K\n" \
    " - Blackmagic Design URSA Mini Pro G2\n" \
    " - Blackmagic Design Pocket Cinema Camera 6K\n" \
    " - Blackmagic URSA Broadcast\n" \
    " - Canon EOS C300 Mark II captured by Blackmagic Video Assist 12G HDR\n" \
    " - Panasonic EVA1 captured by Blackmagic Video Assist 12G HDR\n" \
    " - Sigma fp captured by Blackmagic Video Assist 12G HDR\n\n" \

#define kParamISO "iso"
#define kParamISOLabel "ISO"
#define kParamISOHint "Adjust the ISO"

#define kParamGamma "gamma"
#define kParamGammaLabel "Gamma"
#define kParamGammaHint "Adjust the color space gamma"

#define kParamGamut "gamut"
#define kParamGamutLabel "Color Space"
#define kParamGamutHint "Adjust the color space gamut"

#define kParamRecovery "recovery"
#define kParamRecoveryLabel "Highlight Recovery"
#define kParamRecoveryHint "Enable highlight recovery"
#define kParamRecoveryDefault false

#define kParamColorTemp "colorTemp"
#define kParamColorTempLabel "Color Temp"
#define kParamColorTempHint "Adjust the color temp"

#define kParamTint "tint"
#define kParamTintLabel "Tint"
#define kParamTintHint "Adjust the tint"

#define kParamExposure "exposure"
#define kParamExposureLabel "Exposure"
#define kParamExposureHint "Adjust the exposure"

#define kParamSaturation "saturation"
#define kParamSaturationLabel "Saturation"
#define kParamSaturationHint "Adjust the saturation"

#define kParamContrast "contrast"
#define kParamContrastLabel "Contrast"
#define kParamContrastHint "Adjust the contrast"

#define kParamMidpoint "midpoint"
#define kParamMidpointLabel "Midpoint"
#define kParamMidpointHint "Adjust the midpoint"

#define kParamHighlights "highlights"
#define kParamHighlightsLabel "Highlights"
#define kParamHighlightsHint "Adjust the highlights"

#define kParamShadows "shadows"
#define kParamShadowsLabel "Shadows"
#define kParamShadowsHint "Adjust the shadows"

#define kParamVideoBlackLevel "videoBlackLevel"
#define kParamVideoBlackLevelLabel "Set Video Black Level"
#define kParamVideoBlackLevelHint "Set video black level"
#define kParamVideoBlackLevelDefault false

#define kParamCustomGamma "customGamma"
#define kParamCustomGammaLabel "Custom Gamma"
#define kParamCustomGammaHint "Set custom gamma, will only work with Blackmagick Design Custom."

#define kParamQuality "quality"
#define kParamQualityLabel "Decode Quality"
#define kParamQualityHint "Decoding resolution"
#define kParamQualityDefault BlackmagicRAWHandler::rawFullQuality

using namespace OFX;
using namespace OFX::IO;

#ifdef OFX_IO_USING_OCIO
namespace OCIO = OCIO_NAMESPACE;
#endif

OFXS_NAMESPACE_ANONYMOUS_ENTER

static bool gHostIsNatron = false;
static std::string ofxPath;

class BlackmagicRAWPlugin : public GenericReaderPlugin
{
public:
    BlackmagicRAWPlugin(OfxImageEffectHandle handle,
                        const std::vector<std::string>& extensions);
    virtual ~BlackmagicRAWPlugin();
    virtual void changedParam(const InstanceChangedArgs &args,
                              const std::string &paramName) override final;
    virtual void restoreStateFromParams() override final;
private:
    virtual bool isVideoStream(const std::string& /*filename*/) override final { return true; }
    virtual void decode(const std::string& filename,
                        OfxTime time,
                        int view,
                        bool isPlayback,
                        const OfxRectI& renderWindow,
                        const OfxPointD& renderScale,
                        float *pixelData,
                        const OfxRectI& bounds,
                        PixelComponentEnum pixelComponents,
                        int pixelComponentCount,
                        int rowBytes) override final;
    virtual bool getFrameBounds(const std::string& filename,
                                OfxTime time,
                                int view,
                                OfxRectI *bounds,
                                OfxRectI* format,
                                double *par,
                                std::string *error,
                                int *tile_width,
                                int *tile_height) override final;
    virtual bool guessParamsFromFilename(const std::string& filename,
                                         std::string *colorspace,
                                         PreMultiplicationEnum *filePremult,
                                         PixelComponentEnum *components,
                                         int *componentCount) override final;
    virtual bool getFrameRate(const std::string& filename,
                              double* fps) const override final;
    virtual bool getSequenceTimeDomain(const std::string& filename,
                                       OfxRangeI &range) override final;
    static const std::string getLibraryPath();
    void clearCache();

    OfxRectI _bounds;
    ChoiceParam *_iso;
    ChoiceParam *_gamma;
    ChoiceParam *_gamut;
    BooleanParam *_recovery;
    IntParam *_colorTemp;
    IntParam *_tint;
    DoubleParam *_exposure;
    DoubleParam *_saturation;
    DoubleParam *_contrast;
    DoubleParam *_midpoint;
    DoubleParam *_highlights;
    DoubleParam *_shadows;
    BooleanParam *_videoBlackLevel;
    ChoiceParam *_quality;
};

BlackmagicRAWPlugin::BlackmagicRAWPlugin(OfxImageEffectHandle handle,
                                         const std::vector<std::string>& extensions)
: GenericReaderPlugin(handle,
                      extensions,
                      false,
                      true,
                      false,
                      false,
                      false,
                      false)
, _iso(nullptr)
, _gamma(nullptr)
, _gamut(nullptr)
, _recovery(nullptr)
, _colorTemp(nullptr)
, _tint(nullptr)
, _exposure(nullptr)
, _saturation(nullptr)
, _contrast(nullptr)
, _midpoint(nullptr)
, _highlights(nullptr)
, _shadows(nullptr)
, _videoBlackLevel(nullptr)
, _quality(nullptr)
{
    clearCache();

    _iso = fetchChoiceParam(kParamISO);
    _gamma = fetchChoiceParam(kParamGamma);
    _gamut = fetchChoiceParam(kParamGamut);
    _recovery = fetchBooleanParam(kParamRecovery);
    _colorTemp = fetchIntParam(kParamColorTemp);
    _tint = fetchIntParam(kParamTint);
    _exposure = fetchDoubleParam(kParamExposure);
    _saturation = fetchDoubleParam(kParamSaturation);
    _contrast = fetchDoubleParam(kParamContrast);
    _midpoint = fetchDoubleParam(kParamMidpoint);
    _highlights = fetchDoubleParam(kParamHighlights);
    _shadows = fetchDoubleParam(kParamShadows);
    _videoBlackLevel = fetchBooleanParam(kParamVideoBlackLevel);
    _quality = fetchChoiceParam(kParamQuality);

    assert(_iso && _gamma && _gamma && _recovery && _colorTemp &&
           _tint && _exposure && _saturation && _contrast &&
           _midpoint && _highlights && _shadows && _videoBlackLevel &&
           _quality);

#ifdef _WIN32
    HRESULT result = S_OK;
    result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
}

BlackmagicRAWPlugin::~BlackmagicRAWPlugin()
{
#ifdef _WIN32
    CoUninitialize();
#endif
}

void
BlackmagicRAWPlugin::decode(const std::string& filename,
                            OfxTime time,
                            int /*view*/,
                            bool /*isPlayback*/,
                            const OfxRectI& renderWindow,
                            const OfxPointD& renderScale,
                            float *pixelData,
                            const OfxRectI& /*bounds*/,
                            PixelComponentEnum pixelComponents,
                            int pixelComponentCount,
                            int /*rowBytes*/)
{
    assert(renderScale.x == 1. && renderScale.y == 1.);
    unused(renderScale);
    if (filename.empty() || pixelComponents != ePixelComponentRGB || pixelComponentCount != 3) {
        setPersistentMessage(Message::eMessageError, "", "Wrong input!");
        throwSuiteStatusException(kOfxStatErrFormat);
    }

    // get size
    int width = renderWindow.x2 - renderWindow.x1;
    int height= renderWindow.y2 - renderWindow.y1;

    // set params
    BlackmagicRAWHandler::BlackmagicRAWSpecs specs;
    int iso_selected;
    std::string iso_string;
    _iso->getValue(iso_selected);
    _iso->getOption(iso_selected, iso_string);
    if (!iso_string.empty()) {
        specs.iso = std::stoi(iso_string);
    }
    int gamma_selected;
    _gamma->getValue(gamma_selected);
    _gamma->getOption(gamma_selected, specs.gamma);
    int gamut_selected;
    std::string gamut_string;
    _gamut->getValue(gamut_selected);
    _gamut->getOption(gamut_selected, specs.gamut);
    _recovery->getValue(specs.recovery);
    _colorTemp->getValue(specs.colorTemp);
    _tint->getValue(specs.tint);
    _exposure->getValue(specs.exposure);
    _saturation->getValue(specs.saturation);
    _contrast->getValue(specs.contrast);
    _midpoint->getValue(specs.midpoint);
    _highlights->getValue(specs.highlights);
    _shadows->getValue(specs.shadows);
    _videoBlackLevel->getValue(specs.videoBlackLevel);
    _quality->getValue(specs.quality);

    // setup and run job
    HRESULT result = S_OK;
    IBlackmagicRawFactory* factory = nullptr;
    IBlackmagicRaw* codec = nullptr;
    IBlackmagicRawClip* clip = nullptr;
    IBlackmagicRawJob* readJob = nullptr;
    BlackmagickRAWRendererCallback callback;
    callback.specs = specs;
    bool success = true;

    do {
#ifdef _WIN32
        std::string libpath = getLibraryPath();
        std::wstring wpath(libpath.begin(), libpath.end());
        BSTR libraryPath = SysAllocStringLen(wpath.data(), wpath.size());
        factory = CreateBlackmagicRawFactoryInstanceFromPath(libraryPath);
        SysFreeString(libraryPath);
#elif __APPLE__
        CFStringRef cfpath = CFStringCreateWithCString(kCFAllocatorDefault, getLibraryPath().c_str(), kCFStringEncodingUTF8);
        factory = CreateBlackmagicRawFactoryInstanceFromPath(cfpath);
        //CFRelease(cfpath);
#else
        factory = CreateBlackmagicRawFactoryInstanceFromPath(getLibraryPath().c_str());
#endif
        if (factory == nullptr){
            std::cout << "Failed to create IBlackmagicRawFactory!" << std::endl;
            success = false;
            break;
        }
        result = factory->CreateCodec(&codec);
        if (result != S_OK) {
            std::cout << "Failed to create IBlackmagicRaw!" << std::endl;
            success = false;
            break;
        }
#ifdef _WIN32
        std::wstring wfile(filename.begin(), filename.end());
        BSTR clipName = SysAllocStringLen(wfile.data(), wfile.size());
        result = codec->OpenClip(clipName, &clip);
        SysFreeString(clipName);
#elif __APPLE__
        CFStringRef cffile = CFStringCreateWithCString(kCFAllocatorDefault, filename.c_str(), kCFStringEncodingUTF8);
        result = codec->OpenClip(cffile, &clip);
        //CFRelease(cffile);
#else
        result = codec->OpenClip(filename.c_str(), &clip);
#endif
        if (result != S_OK) {
            std::cout << "Failed to open IBlackmagicRawClip!" << std::endl;
            success = false;
            break;
        }
        callback.clip = clip;
        result = codec->SetCallback(&callback);
        if (result != S_OK) {
            std::cout << "Failed to set IBlackmagicRawCallback!" << std::endl;
            success = false;
            break;
        }
        result = clip->CreateJobReadFrame(time>0?time-1:0, &readJob);
        if (result != S_OK) {
            std::cout << "Failed to create IBlackmagicRawJob!" << std::endl;
            success = false;
            break;
        }
        result = readJob->Submit();
        if (result != S_OK) {
            readJob->Release();
            std::cout << "Failed to submit IBlackmagicRawJob!" << std::endl;
            success = false;
            break;
        }
        codec->FlushJobs();
    } while(0);

    if (callback.frameBuffer == nullptr || !success) {
        if (clip != nullptr) { clip->Release(); }
        if (codec != nullptr) { codec->Release(); }
        if (factory != nullptr) { factory->Release(); }
        setPersistentMessage(Message::eMessageError, "", "Unable to read or render image");
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    float* buffer = (float*)callback.frameBuffer;
    int offset = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixelData[offset + 0] = buffer[offset + 0];
            pixelData[offset + 1] = buffer[offset + 1];
            pixelData[offset + 2] = buffer[offset + 2];
            offset += pixelComponentCount;
        }
    }

    if (clip != nullptr) { clip->Release(); }
    if (codec != nullptr) { codec->Release(); }
    if (factory != nullptr) { factory->Release(); }
    callback.frameBuffer = nullptr;
    callback.clip = nullptr;
    buffer = nullptr;
}

bool BlackmagicRAWPlugin::getFrameBounds(const std::string& filename,
                                         OfxTime /*time*/,
                                         int /*view*/,
                                         OfxRectI *bounds,
                                         OfxRectI* format,
                                         double *par,
                                         std::string* /*error*/,
                                         int *tile_width,
                                         int *tile_height)
{
    int width = 0;
    int height = 0;
    if (_bounds.x2 > 0 && _bounds.y2 > 0) { // use cache since this function run several time for each frame
        width = _bounds.x2;
        height = _bounds.y2;
    }
    if (width == 0 && height == 0) {
        BlackmagicRAWHandler::BlackmagicRAWSpecs specs;
        specs = BlackmagicRAWHandler::getClipSpecs(filename, getLibraryPath());
        int clipWidth = specs.width;
        int clipHeight = specs.height;
        if (clipWidth > 0 && clipHeight > 0) {
            int quality;
            _quality->getValue(quality);
            switch (quality) {
            case BlackmagicRAWHandler::rawHalfQuality:
                width = (int)clipWidth/2;
                height = (int)clipHeight/2;
                break;
            case BlackmagicRAWHandler::rawQuarterQuality:
                width = (int)clipWidth/4;
                height = (int)clipHeight/4;
                break;
            case BlackmagicRAWHandler::rawEighthQuality:
                width = (int)clipWidth/8;
                height = (int)clipHeight/8;
                break;
            default:
                width = (int)clipWidth;
                height = (int)clipHeight;
            }
        }
    }
    if (width > 0 && height > 0) {
        bounds->x1 = 0;
        bounds->x2 = width;
        _bounds.x2 = width; // cache
        bounds->y1 = 0;
        bounds->y2 = height;
        _bounds.y2 = height; // cache
        *format = *bounds;
        *par = 1.0;
    }
    *tile_width = *tile_height = 0;
    return true;
}

bool BlackmagicRAWPlugin::guessParamsFromFilename(const std::string& /*filename*/,
                                                  std::string *colorspace,
                                                  PreMultiplicationEnum *filePremult,
                                                  PixelComponentEnum *components,
                                                  int *componentCount)
{
    assert(colorspace && filePremult && components && componentCount);
# ifdef OFX_IO_USING_OCIO
    // Unless otherwise specified, video files are assumed to be rec709.
    if ( _ocio->hasColorspace("Rec709") ) {
        // nuke-default
        *colorspace = "Rec709";
    } else if ( _ocio->hasColorspace("nuke_rec709") ) {
        // blender
        *colorspace = "nuke_rec709";
    } else if ( _ocio->hasColorspace("Rec.709 - Full") ) {
        // out_rec709full or "Rec.709 - Full" in aces 1.0.0
        *colorspace = "Rec.709 - Full";
    } else if ( _ocio->hasColorspace("out_rec709full") ) {
        // out_rec709full or "Rec.709 - Full" in aces 1.0.0
        *colorspace = "out_rec709full";
    } else if ( _ocio->hasColorspace("rrt_rec709_full_100nits") ) {
        // rrt_rec709_full_100nits in aces 0.7.1
        *colorspace = "rrt_rec709_full_100nits";
    } else if ( _ocio->hasColorspace("rrt_rec709") ) {
        // rrt_rec709 in aces 0.1.1
        *colorspace = "rrt_rec709";
    } else if ( _ocio->hasColorspace("hd10") ) {
        // hd10 in spi-anim and spi-vfx
        *colorspace = "hd10";
    }
# endif
    *components = ePixelComponentRGB;
    return true;
}

bool BlackmagicRAWPlugin::getFrameRate(const std::string &filename,
                                       double *fps) const
{
    assert(fps);
    BlackmagicRAWHandler::BlackmagicRAWSpecs specs;
    specs = BlackmagicRAWHandler::getClipSpecs(filename, getLibraryPath());
    *fps = specs.fps;
    return true;
}

bool BlackmagicRAWPlugin::getSequenceTimeDomain(const std::string &filename,
                                                OfxRangeI &range)
{
    //std::cout << "getSequenceTimeDomain " << filename << std::endl;
    if (!filename.empty()) {
        BlackmagicRAWHandler::BlackmagicRAWSpecs specs;
        specs = BlackmagicRAWHandler::getClipSpecs(filename, getLibraryPath());
        if (specs.frameMax > 0) {
            range.min = 1;
            range.max = specs.frameMax;
        }
    }
    return true;
}

const std::string BlackmagicRAWPlugin::getLibraryPath()
{
    // TODO: we should check if file exists
    std::string result = ofxPath;
    result.append("/Contents/Resources/BlackmagicRAW");
    //"/usr/lib/blackmagic/BlackmagicRAWSDK/Linux/Libraries";
    //"/Applications/Blackmagic\ RAW/Blackmagic\ RAW\ SDK/Mac/Libraries";
    return result;
}

void BlackmagicRAWPlugin::clearCache()
{
    _bounds.x1 = 0;
    _bounds.x2 = -1;
    _bounds.y1 = 0;
    _bounds.y2 = -1;
}
void BlackmagicRAWPlugin::changedParam(const InstanceChangedArgs &args,
                                       const std::string &paramName)
{
    if (paramName == kParamQuality) {
        clearCache();
    }
    else { GenericReaderPlugin::changedParam(args, paramName); }
}

void BlackmagicRAWPlugin::restoreStateFromParams()
{
    clearCache();
    std::string filename;
    _fileParam->getValue(filename);
    if (!filename.empty()) {
        BlackmagicRAWHandler::BlackmagicRAWSpecs specs;
        specs = BlackmagicRAWHandler::getClipSpecs(filename, getLibraryPath());

        _iso->resetOptions(specs.availableISO);
        if (specs.iso > 0) {
            for (uint32_t i = 0; i < specs.availableISO.size(); ++i) {
                int currentISO = std::stoi(specs.availableISO.at(i));
                if ( currentISO == specs.iso) {
                    _iso->setDefault(i);
                    _iso->resetToDefault();
                    break;
                }
            }
        }

        _gamma->resetOptions(specs.availableGamma);
        if (!specs.gamma.empty()) {
            for (uint32_t i = 0; i < specs.availableGamma.size(); ++i) {
                if (specs.availableGamma.at(i) == specs.gamma) {
                    _gamma->setDefault(i);
                    _gamma->resetToDefault();
                    break;
                }
            }
        }

        _gamut->resetOptions(specs.availableGamut);
        if (!specs.gamut.empty()) {
            for (uint32_t i = 0; i < specs.availableGamut.size(); ++i) {
                if (specs.availableGamut.at(i) == specs.gamut) {
                    _gamut->setDefault(i);
                    _gamut->resetToDefault();
                    break;
                }
            }
        }

        _colorTemp->setDefault(specs.colorTemp);
        _colorTemp->setValue(specs.colorTemp);

        _tint->setDefault(specs.tint);
        _tint->setValue(specs.tint);

        _exposure->setDefault(specs.exposure);
        _exposure->setValue(specs.exposure);

        _saturation->setDefault(specs.saturation);
        _saturation->setValue(specs.saturation);

        _contrast->setDefault(specs.contrast);
        _contrast->setValue(specs.contrast);

        _midpoint->setDefault(specs.midpoint);
        _midpoint->setValue(specs.midpoint);

        _highlights->setDefault(specs.highlights);
        _highlights->setValue(specs.highlights);

        _shadows->setDefault(specs.shadows);
        _shadows->setValue(specs.shadows);

        _videoBlackLevel->setDefault(specs.videoBlackLevel);
        _videoBlackLevel->setValue(specs.videoBlackLevel);
    }
    GenericReaderPlugin::restoreStateFromParams();
}

mDeclareReaderPluginFactory(BlackmagicRAWPluginFactory, {}, true);

void BlackmagicRAWPluginFactory::load()
{
    _extensions.clear();
    _extensions.push_back("braw");
    _extensions.push_back("sidecar");
}

void BlackmagicRAWPluginFactory::describe(ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc,
                          _extensions,
                          kPluginEvaluation,
                          false,
                          false);
    desc.setLabel(kPluginName);
    desc.setPluginDescription(kPluginDescription);
}

void BlackmagicRAWPluginFactory::describeInContext(ImageEffectDescriptor &desc,
                                                   ContextEnum context)
{
    gHostIsNatron = (getImageEffectHostDescription()->isNatron);
    ofxPath = desc.getPropertySet().propGetString(kOfxPluginPropFilePath, false);
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc,
                                                                    context,
                                                                    true,
                                                                    false,
                                                                    true,
                                                                    false,
                                                                    false,
                                                                    false,
                                                                    true);
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamQuality);
        param->setLabel(kParamQualityLabel);
        param->setHint(kParamQualityHint);
        param->appendOption("Full");
        param->appendOption("Half");
        param->appendOption("Quarter");
        param->appendOption("Eight");
        param->setDefault(kParamQualityDefault);
        if (page) { page->addChild(*param); }
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamGamut);
        param->setLabel(kParamGamutLabel);
        param->setHint(kParamGamutHint);
        if (page) { page->addChild(*param); }
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamGamma);
        param->setLabel(kParamGammaLabel);
        param->setHint(kParamGammaHint);
        if (page) { page->addChild(*param); }
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamISO);
        param->setLabel(kParamISOLabel);
        param->setHint(kParamISOHint);
        if (page) { page->addChild(*param); }
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamRecovery);
        param->setLabel(kParamRecoveryLabel);
        param->setHint(kParamRecoveryHint);
        param->setDefault(kParamRecoveryDefault);
        param->setLayoutHint(eLayoutHintDivider);
        if (page) { page->addChild(*param); }
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamColorTemp);
        param->setLabel(kParamColorTempLabel);
        param->setHint(kParamColorTempHint);
        param->setRange(2000, 10000);
        if (page) { page->addChild(*param); }
    }
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamTint);
        param->setLabel(kParamTintLabel);
        param->setHint(kParamTintHint);
        param->setRange(-100, 100);
        if (page) { page->addChild(*param); }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamExposure);
        param->setLabel(kParamExposureLabel);
        param->setHint(kParamExposureHint);
        param->setRange(-5.0, 5.0);
        if (page) { page->addChild(*param); }
    }
    GroupParamDescriptor* group = desc.defineGroupParam(kParamCustomGamma);
    if (group) {
        group->setLabel(kParamCustomGammaLabel);
        group->setHint(kParamCustomGammaHint);
        group->setLayoutHint(eLayoutHintDivider);
        group->setOpen(true);
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSaturation);
        param->setLabel(kParamSaturationLabel);
        param->setHint(kParamSaturationHint);
        param->setRange(0.0, 2.0);
        if (group) { param->setParent(*group); }
        if (page) { page->addChild(*param); }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamContrast);
        param->setLabel(kParamContrastLabel);
        param->setHint(kParamContrastHint);
        param->setRange(0.0, 2.0);
        if (group) { param->setParent(*group); }
        if (page) { page->addChild(*param); }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamMidpoint);
        param->setLabel(kParamMidpointLabel);
        param->setHint(kParamMidpointHint);
        param->setRange(0.0, 1.0);
        if (group) { param->setParent(*group); }
        if (page) { page->addChild(*param); }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamHighlights);
        param->setLabel(kParamHighlightsLabel);
        param->setHint(kParamHighlightsHint);
        param->setRange(0.0, 2.0);
        if (group) { param->setParent(*group); }
        if (page) { page->addChild(*param); }
    }
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamShadows);
        param->setLabel(kParamShadowsLabel);
        param->setHint(kParamShadowsHint);
        param->setRange(0.0, 2.0);
        if (group) { param->setParent(*group); }
        if (page) { page->addChild(*param); }
    }
    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamVideoBlackLevel);
        param->setLabel(kParamVideoBlackLevelLabel);
        param->setHint(kParamVideoBlackLevelHint);
        param->setDefault(kParamVideoBlackLevelDefault);
        if (group) { param->setParent(*group); }
        if (page) { page->addChild(*param); }
    }
    GenericReaderDescribeInContextEnd(desc,
                                      context,
                                      page,
                                      "rec709",
                                      "scene_linear");
}

ImageEffect* BlackmagicRAWPluginFactory::createInstance(OfxImageEffectHandle handle,
                                                        ContextEnum /*context*/)
{
    BlackmagicRAWPlugin* ret = new BlackmagicRAWPlugin(handle, _extensions);
    ret->restoreStateFromParams();
    return ret;
}

static BlackmagicRAWPluginFactory p(kPluginIdentifier,
                                    kPluginVersionMajor,
                                    kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
