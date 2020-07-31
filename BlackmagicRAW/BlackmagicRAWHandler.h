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

#ifdef _WIN32
#include "BlackmagicRawAPIDispatch.h"
#else
#include "BlackmagicRawAPI.h"
#endif
#include <iostream>
#include <string>
#include <vector>

#ifdef DEBUG
    #include <cassert>
    #define VERIFY(condition) assert(SUCCEEDED(condition))
#else
    #define VERIFY(condition) condition
#endif

static const BlackmagicRawResourceFormat s_resourceFormat = blackmagicRawResourceFormatRGBF32;

class BlackmagicRAWHandler
{
public:
    enum BlackmagicRAWQuality
    {
        rawFullQuality,
        rawHalfQuality,
        rawQuarterQuality,
        rawEighthQuality
    };
    struct BlackmagicRAWSpecs
    {
        int quality = rawFullQuality;
        int width = 0;
        int height = 0;
        double fps = 0;
        int frameMax = 0;
        std::string gamut;
        std::string gamma;
        int iso = 0;
        bool recovery = false;
        int colorTemp = 0;
        int tint = 0;
        double exposure = 0;
        double saturation = 0;
        double contrast = 0;
        double midpoint = 0;
        double highlights = 0;
        double shadows = 0;
        double whiteLevel = 0;
        double blackLevel = 0;
        bool videoBlackLevel = false;
        std::vector<std::string> availableISO;
        std::vector<std::string> availableGamma;
        std::vector<std::string> availableGamut;
    };
    static const BlackmagicRAWSpecs getClipSpecs(const std::string &filename,
                                                 const std::string &path);
};

class BlackmagickRAWSpecsCallback : public IBlackmagicRawCallback
{
public:
    explicit BlackmagickRAWSpecsCallback() = default;
    virtual ~BlackmagickRAWSpecsCallback() = default;
    BlackmagicRAWHandler::BlackmagicRAWSpecs specs;
    virtual void ReadComplete(IBlackmagicRawJob* readJob,
                              HRESULT result,
                              IBlackmagicRawFrame* frame);
    virtual void ProcessComplete(IBlackmagicRawJob* job,
                                 HRESULT result,
                                 IBlackmagicRawProcessedImage* processedImage);
    virtual void DecodeComplete(IBlackmagicRawJob*, HRESULT) {}
    virtual void TrimProgress(IBlackmagicRawJob*, float) {}
    virtual void TrimComplete(IBlackmagicRawJob*, HRESULT) {}
#ifdef _WIN32
    virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*,
                                             BSTR,
                                             uint32_t,
                                             BSTR) {}
    virtual void SidecarMetadataParseError(IBlackmagicRawClip*,
                                           BSTR,
                                           uint32_t,
                                           BSTR) {}
#elif __APPLE__
    virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*,
                                             CFStringRef,
                                             uint32_t,
                                             CFStringRef) {}
    virtual void SidecarMetadataParseError(IBlackmagicRawClip*,
                                           CFStringRef,
                                           uint32_t,
                                           CFStringRef) {}
#else
    virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*,
                                             const char*,
                                             uint32_t,
                                             const char*) {}
    virtual void SidecarMetadataParseError(IBlackmagicRawClip*,
                                           const char*,
                                           uint32_t,
                                           const char*) {}
#endif
    virtual void PreparePipelineComplete(void*, HRESULT) {}
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) { return E_NOTIMPL; }
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return 0; }
};

class BlackmagickRAWRendererCallback : public IBlackmagicRawCallback
{
public:
    explicit BlackmagickRAWRendererCallback() = default;
    virtual ~BlackmagickRAWRendererCallback() = default;
    IBlackmagicRawClip *clip = nullptr;
    void *frameBuffer = nullptr;
    BlackmagicRAWHandler::BlackmagicRAWSpecs specs;
    virtual void ReadComplete(IBlackmagicRawJob* readJob,
                              HRESULT result,
                              IBlackmagicRawFrame* frame);
    virtual void ProcessComplete(IBlackmagicRawJob* job,
                                 HRESULT result,
                                 IBlackmagicRawProcessedImage* processedImage);
    virtual void DecodeComplete(IBlackmagicRawJob*, HRESULT) {}
    virtual void TrimProgress(IBlackmagicRawJob*, float) {}
    virtual void TrimComplete(IBlackmagicRawJob*, HRESULT) {}
#ifdef _WIN32
    virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*,
                                             BSTR,
                                             uint32_t,
                                             BSTR) {}
    virtual void SidecarMetadataParseError(IBlackmagicRawClip*,
                                           BSTR,
                                           uint32_t,
                                           BSTR) {}
#elif __APPLE__
    virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*,
                                             CFStringRef,
                                             uint32_t,
                                             CFStringRef) {}
    virtual void SidecarMetadataParseError(IBlackmagicRawClip*,
                                           CFStringRef,
                                           uint32_t,
                                           CFStringRef) {}
#else
    virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*,
                                             const char*,
                                             uint32_t,
                                             const char*) {}
    virtual void SidecarMetadataParseError(IBlackmagicRawClip*,
                                           const char*,
                                           uint32_t,
                                           const char*) {}
#endif
    virtual void PreparePipelineComplete(void*, HRESULT) {}
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) { return E_NOTIMPL; }
    virtual ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
    virtual ULONG STDMETHODCALLTYPE Release(void) { return 0; }
};
