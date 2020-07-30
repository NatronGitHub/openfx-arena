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

#ifdef _WIN32
#include "BlackmagicRawAPI_i.c"
#define BMVAR VARIANT
#else
#define BMVAR Variant
#endif

const BlackmagicRAWHandler::BlackmagicRAWSpecs
BlackmagicRAWHandler::getClipSpecs(const std::string &filename,
                                   const std::string &path)
{
    HRESULT result = S_OK;
    BlackmagicRAWSpecs specs;

    if (filename.empty() || path.empty()) { return specs; }

    IBlackmagicRawFactory* factory = nullptr;
    IBlackmagicRaw* codec = nullptr;
    IBlackmagicRawClip* clip = nullptr;
    IBlackmagicRawJob* readJob = nullptr;
    IBlackmagicRawConstants* constants = nullptr;
    IBlackmagicRawClipProcessingAttributes *clipAttr = nullptr;
    BlackmagickRAWSpecsCallback callback;

#ifdef _WIN32
    BSTR cameraType;
#else
    const char* cameraType;
#endif

    do {
        // setup factory
#ifdef _WIN32
        std::wstring wpath(path.begin(), path.end());
        BSTR libraryPath = SysAllocStringLen(wpath.data(), wpath.size());
        factory = CreateBlackmagicRawFactoryInstanceFromPath(libraryPath);
        SysFreeString(libraryPath);
#else
        factory = CreateBlackmagicRawFactoryInstanceFromPath(path.c_str());
#endif
        if (factory == nullptr){
            std::cout << "Failed to create IBlackmagicRawFactory!" << std::endl;
            break;
        }

        // get codecs
        result = factory->CreateCodec(&codec);
        if (result != S_OK) {
            std::cout << "Failed to create IBlackmagicRaw!" << std::endl;
            break;
        }

        // get clip
#ifdef _WIN32
        std::wstring wfile(filename.begin(), filename.end());
        BSTR clipName = SysAllocStringLen(wfile.data(), wfile.size());
        result = codec->OpenClip(clipName, &clip);
        SysFreeString(clipName);
#else
        result = codec->OpenClip(filename.c_str(), &clip);
#endif
        if (result != S_OK) {
            std::cout << "Failed to open IBlackmagicRawClip!" << std::endl;
            break;
        }

        // get camera type
        result = clip->GetCameraType(&cameraType);
        if (result != S_OK) {
            std::cout << "Failed to get camera type" << std::endl;
            break;
        }

        // get constants
        result = codec->QueryInterface(IID_IBlackmagicRawConstants, (void**)&constants);
        if (result != S_OK) {
            std::cout << "Failed to get constants" << std::endl;
            break;
        }

        // get gamut
#ifdef _WIN32
        BOOL isReadOnly = false;
#else
        bool isReadOnly = false;
#endif
        BMVAR colorGamutListVar[20];
        uint32_t colorGamutListLen;
        result = constants->GetClipProcessingAttributeList(cameraType,
                                                           blackmagicRawClipProcessingAttributeGamut,
                                                           colorGamutListVar,
                                                           &colorGamutListLen,
                                                           &isReadOnly);
        if (result == S_OK && colorGamutListLen) {
            for (uint32_t i = 0; i < colorGamutListLen; ++i) {
#ifdef _WIN32
                BSTR bval = colorGamutListVar[i].bstrVal;
                std::wstring wval(bval, SysStringLen(bval));
                SysFreeString(bval);
                std::string val(wval.begin(), wval.end());
                specs.availableGamut.push_back(val);
#else
                specs.availableGamut.push_back(colorGamutListVar[i].bstrVal);
#endif
            }
        }
        BMVAR defaultGamut;
#ifdef _WIN32
        std::wstring wvgamut = L"viewing_gamut";
        BSTR vgamut = SysAllocStringLen(wvgamut.data(), wvgamut.size());;
        result = clip->GetMetadata(vgamut, &defaultGamut);
        SysFreeString(vgamut);
        std::wstring defaultGamutWString(defaultGamut.bstrVal, SysStringLen(defaultGamut.bstrVal));
        std::string defaultGamutString(defaultGamutWString.begin(), defaultGamutWString.end());
        specs.gamut = defaultGamutString;
#else
        result = clip->GetMetadata("viewing_gamut", &defaultGamut);
        specs.gamut = defaultGamut.bstrVal;
#endif

        // get gamma
        BMVAR colorGammaListVar[20];
        uint32_t colorGammaListLen;
        result = constants->GetClipProcessingAttributeList(cameraType,
                                                           blackmagicRawClipProcessingAttributeGamma,
                                                           colorGammaListVar,
                                                           &colorGammaListLen,
                                                           &isReadOnly);
        if (result == S_OK && colorGammaListLen) {
            for (uint32_t i = 0; i < colorGammaListLen; ++i) {
#ifdef _WIN32
                BSTR bval = colorGammaListVar[i].bstrVal;
                std::wstring wval(bval, SysStringLen(bval));
                SysFreeString(bval);
                std::string val(wval.begin(), wval.end());
                specs.availableGamma.push_back(val);
#else
                specs.availableGamma.push_back(colorGammaListVar[i].bstrVal);
#endif
            }
        }
        BMVAR defaultGamma;
#ifdef _WIN32
        std::wstring wvgamma = L"viewing_gamma";
        BSTR vgamma = SysAllocStringLen(wvgamma.data(), wvgamma.size());
        result = clip->GetMetadata(vgamma, &defaultGamma);
        SysFreeString(vgamma);
        std::wstring defaultGammaWString(defaultGamma.bstrVal, SysStringLen(defaultGamma.bstrVal));
        std::string defaultGammaString(defaultGammaWString.begin(), defaultGammaWString.end());
        specs.gamma = defaultGammaString;
#else
        result = clip->GetMetadata("viewing_gamma", &defaultGamma);
        specs.gamma = defaultGamma.bstrVal;
#endif

        // get available iso
        BMVAR isoListVar[20];
        uint32_t isoListLen;
        result = constants->GetFrameProcessingAttributeList(cameraType,
                                                            blackmagicRawFrameProcessingAttributeISO,
                                                            isoListVar,
                                                            &isoListLen,
                                                            &isReadOnly);
        if (result == S_OK && isoListLen) {
            for (uint32_t i = 0; i < isoListLen; ++i) {
                specs.availableISO.push_back(std::to_string(isoListVar[i].uiVal));
            }
        }

        // get image size
        uint32_t clipWidth = 0;
        uint32_t clipHeight = 0;
        result = clip->GetWidth(&clipWidth);
        result = clip->GetHeight(&clipHeight);
        if (clipWidth > 0 && clipHeight > 0) {
            specs.width = clipWidth;
            specs.height = clipHeight;
        }

        // get frame rate
        float frameRate = 0;
        result = clip->GetFrameRate(&frameRate);
        specs.fps = frameRate;

        // get frame count
        uint64_t frameCount = 0;
        clip->GetFrameCount(&frameCount);
        specs.frameMax = frameCount;

        // get clip attributes
        result = clip->CloneClipProcessingAttributes(&clipAttr);
        if (result != S_OK) {
            std::cout << "Failed to get IBlackmagicRawClipProcessingAttributes!" << std::endl;
            break;
        }

        // custom gamut options
        BMVAR saturation;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveSaturation,
                                            &saturation);
        specs.saturation = saturation.fltVal;

        BMVAR contrast;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveContrast,
                                            &contrast);
        specs.contrast = contrast.fltVal;

        BMVAR midpoint;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveMidpoint,
                                            &midpoint);
        specs.midpoint = midpoint.fltVal;

        BMVAR highlights;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveHighlights,
                                            &highlights);
        specs.highlights = highlights.fltVal;

        BMVAR shadows;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveShadows,
                                            &shadows);
        specs.shadows = shadows.fltVal;

        BMVAR videoBlackLevel;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveVideoBlackLevel,
                                            &videoBlackLevel);
        specs.videoBlackLevel = videoBlackLevel.uiVal==1?true:false;

        BMVAR blackLevel;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveBlackLevel,
                                            &blackLevel);
        specs.blackLevel = blackLevel.fltVal;

        BMVAR whiteLevel;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveWhiteLevel,
                                            &whiteLevel);
        specs.whiteLevel = whiteLevel.fltVal;

        // set callback
        result = codec->SetCallback(&callback);
        if (result != S_OK) {
            std::cout << "Failed to set IBlackmagicRawCallback!" << std::endl;
            break;
        }

        // create job
        result = clip->CreateJobReadFrame(0, &readJob);
        if (result != S_OK) {
            std::cout << "Failed to create IBlackmagicRawJob!" << std::endl;
            break;
        }

        // process job
        result = readJob->Submit();
        if (result != S_OK) {
            readJob->Release();
            std::cout << "Failed to submit IBlackmagicRawJob!" << std::endl;
            break;
        }
        codec->FlushJobs();
    } while(0);

    // add frame specs from job
    specs.colorTemp = callback.specs.colorTemp;
    specs.tint = callback.specs.tint;
    specs.exposure = callback.specs.exposure;
    specs.iso = callback.specs.iso;

    if (clipAttr != nullptr) { clipAttr->Release(); }
    if (constants != nullptr) { constants->Release(); }
    if (clip != nullptr) { clip->Release(); }
    if (codec != nullptr) { codec->Release(); }
    if (factory != nullptr) { factory->Release(); }
    cameraType = nullptr;
#ifdef _WIN32
        SysFreeString(cameraType);
#endif
    return specs;
}

void BlackmagickRAWSpecsCallback::ReadComplete(IBlackmagicRawJob *readJob,
                                               HRESULT result,
                                               IBlackmagicRawFrame *frame)
{
    if (result == S_OK) {
        VERIFY(frame->SetResourceFormat(s_resourceFormat));
    }

    IBlackmagicRawFrameProcessingAttributes *frameAttr;
    result = frame->CloneFrameProcessingAttributes(&frameAttr);

    if (result == S_OK) {
        BMVAR colorTempVar, tintVar, exposureVar, isoVar;
        result = frameAttr->GetFrameAttribute(blackmagicRawFrameProcessingAttributeWhiteBalanceKelvin,
                                              &colorTempVar); // uintVal
        result = frameAttr->GetFrameAttribute(blackmagicRawFrameProcessingAttributeWhiteBalanceTint,
                                              &tintVar); // uiVal
        result = frameAttr->GetFrameAttribute(blackmagicRawFrameProcessingAttributeExposure,
                                              &exposureVar); // fltVal
        result = frameAttr->GetFrameAttribute(blackmagicRawFrameProcessingAttributeISO,
                                              &isoVar); // uintVal
        specs.colorTemp = colorTempVar.uintVal;
        specs.tint = tintVar.uiVal;
        specs.exposure = exposureVar.fltVal;
        specs.iso = isoVar.uintVal;
    }

    readJob->Release();
    if (frameAttr != nullptr) { frameAttr->Release(); }
}

void BlackmagickRAWSpecsCallback::ProcessComplete(IBlackmagicRawJob *job,
                                                  HRESULT /*result*/,
                                                  IBlackmagicRawProcessedImage* /*processedImage*/)
{
    if (job != nullptr) { job->Release(); }
}

void BlackmagickRAWRendererCallback::ReadComplete(IBlackmagicRawJob *readJob,
                                                  HRESULT result,
                                                  IBlackmagicRawFrame *frame)
{
    if (clip == nullptr) { return; }
    IBlackmagicRawJob* decodeAndProcessJob = nullptr;
    if (result == S_OK) {
        VERIFY(frame->SetResourceFormat(s_resourceFormat));
    }

    // get and set attributes
    IBlackmagicRawFrameProcessingAttributes *frameAttr = nullptr;
    IBlackmagicRawClipProcessingAttributes *clipAttr = nullptr;
    result = frame->CloneFrameProcessingAttributes(&frameAttr);
    result = clip->CloneClipProcessingAttributes(&clipAttr);

    if (result == S_OK) {
        BMVAR iso;
        result = frameAttr->GetFrameAttribute(blackmagicRawFrameProcessingAttributeISO,
                                              &iso);
        iso.uintVal = specs.iso;
        result = frameAttr->SetFrameAttribute(blackmagicRawFrameProcessingAttributeISO,
                                              &iso);
    }
    if (result == S_OK) {
        BMVAR colorTemp;
        result = frameAttr->GetFrameAttribute(blackmagicRawFrameProcessingAttributeWhiteBalanceKelvin,
                                              &colorTemp);
        colorTemp.uintVal = specs.colorTemp;
        result = frameAttr->SetFrameAttribute(blackmagicRawFrameProcessingAttributeWhiteBalanceKelvin,
                                              &colorTemp);
    }
    if (result == S_OK) {
        BMVAR tint;
        result = frameAttr->GetFrameAttribute(blackmagicRawFrameProcessingAttributeWhiteBalanceTint,
                                              &tint);
        tint.uiVal = specs.tint;
        result = frameAttr->SetFrameAttribute(blackmagicRawFrameProcessingAttributeWhiteBalanceTint,
                                              &tint);
    }
    if (result == S_OK) {
        BMVAR exposure;
        result = frameAttr->GetFrameAttribute(blackmagicRawFrameProcessingAttributeExposure,
                                              &exposure);
        exposure.fltVal = specs.exposure;
        result = frameAttr->SetFrameAttribute(blackmagicRawFrameProcessingAttributeExposure,
                                              &exposure);
    }
    if (result == S_OK) {
        BMVAR gamut;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeGamut,
                                            &gamut);
#ifdef _WIN32
        std::wstring wgamut(specs.gamut.begin(), specs.gamut.end());
        BSTR bgamut = SysAllocStringLen(wgamut.data(), wgamut.size());
        gamut.bstrVal = bgamut;
        SysFreeString(bgamut);
#else
        gamut.bstrVal = specs.gamut.c_str();
#endif
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeGamut,
                                            &gamut);
    }
    if (result == S_OK) {
        BMVAR gamma;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeGamma,
                                            &gamma);
#ifdef _WIN32
        std::wstring wgamma(specs.gamma.begin(), specs.gamma.end());
        BSTR bgamma = SysAllocStringLen(wgamma.data(), wgamma.size());
        gamma.bstrVal = bgamma;
        SysFreeString(bgamma);
#else
        gamma.bstrVal = specs.gamma.c_str();
#endif
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeGamma,
                                            &gamma);
    }
    if (result == S_OK) {
        BMVAR hrecovery;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeHighlightRecovery,
                                            &hrecovery);
        hrecovery.uiVal = specs.recovery? 1 : 0;
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeHighlightRecovery,
                                            &hrecovery);
    }
    if (result == S_OK) {
        BMVAR saturation;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveSaturation,
                                            &saturation);
        saturation.fltVal = specs.saturation;
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveSaturation,
                                            &saturation);
    }
    if (result == S_OK) {
        BMVAR contrast;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveContrast,
                                            &contrast);
        contrast.fltVal = specs.contrast;
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveContrast,
                                            &contrast);
    }
    if (result == S_OK) {
        BMVAR midpoint;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveMidpoint,
                                            &midpoint);
        midpoint.fltVal = specs.midpoint;
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveMidpoint,
                                            &midpoint);
    }
    if (result == S_OK) {
        BMVAR highlights;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveHighlights,
                                            &highlights);
        highlights.fltVal = specs.highlights;
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveHighlights,
                                            &highlights);
    }
    if (result == S_OK) {
        BMVAR shadows;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveShadows,
                                            &shadows);
        shadows.fltVal = specs.shadows;
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveShadows,
                                            &shadows);
    }
    if (result == S_OK) {
        BMVAR videoBlackLevel;
        result = clipAttr->GetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveVideoBlackLevel,
                                            &videoBlackLevel);
        videoBlackLevel.uiVal = specs.videoBlackLevel?1:0;
        result = clipAttr->SetClipAttribute(blackmagicRawClipProcessingAttributeToneCurveVideoBlackLevel,
                                            &videoBlackLevel);
    }

    // set quality (scale)
    switch (specs.quality) {
    case BlackmagicRAWHandler::rawHalfQuality:
        result = frame->SetResolutionScale(blackmagicRawResolutionScaleHalfUpsideDown);
        break;
    case BlackmagicRAWHandler::rawQuarterQuality:
        result = frame->SetResolutionScale(blackmagicRawResolutionScaleQuarterUpsideDown);
        break;
    case BlackmagicRAWHandler::rawEighthQuality:
        result = frame->SetResolutionScale(blackmagicRawResolutionScaleEighthUpsideDown);
        break;
    default:
        result = frame->SetResolutionScale(blackmagicRawResolutionScaleFullUpsideDown);
    }

    // setup and run job
    if (result == S_OK) {
        result = frame->CreateJobDecodeAndProcessFrame(clipAttr,
                                                       frameAttr,
                                                       &decodeAndProcessJob);
    }
    if (result == S_OK) {
        result = decodeAndProcessJob->Submit();
    }
    if (result != S_OK){
        if (decodeAndProcessJob) {
            decodeAndProcessJob->Release();
        }
    }
    readJob->Release();
    if (frameAttr != nullptr) { frameAttr->Release(); }
    if (clipAttr != nullptr) { clipAttr->Release(); }
}

void BlackmagickRAWRendererCallback::ProcessComplete(IBlackmagicRawJob *job,
                                                     HRESULT result,
                                                     IBlackmagicRawProcessedImage *processedImage)
{
    if (result == S_OK) {
        result = processedImage->GetResource(&frameBuffer);
    }
    job->Release();
}

