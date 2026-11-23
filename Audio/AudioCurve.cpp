/*
####################################################################
#
# OpenFX Audio Curve Generator
#
# Copyright (C) 2019 Ole-André Rodlie <ole.andre.rodlie@gmail.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
####################################################################
*/

#include "sox.h"
#include "ofxsImageEffect.h"
#include "PackedImage.h"

#include <stdint.h>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <climits>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#define kPluginName "AudioCurveOFX"
#define kPluginIdentifier "net.fxarena.openfx.AudioCurve"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0
#define kPluginGrouping "Other"
#define kPluginDescription "Generate curve data from (stereo) audio files."

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 0
#define kRenderThreadSafety eRenderFullySafe

#define kParamFile "audio"
#define kParamFileLabel "Audio File"
#define kParamFileHint "Audio file used to generate curve data."

#define kParamFPS "fps"
#define kParamFPSLabel "Frame Rate"
#define kParamFPSHint "The frame rate of the project."
#define kParamFPSDefault 24.0
#define kParamFPSMin 1.
#define kParamFPSMax 1000.

#define kParamFrames "frames"
#define kParamFramesLabel "Frame Range"
#define kParamFramesHint "The desired frame range."
#define kParamFramesDefault 250

#define kParamFactor "factor"
#define kParamFactorLabel "Curve Height"
#define kParamFactorHint "Adjust the curve height."
#define kParamFactorDefault 100

#define kParamZero "zero"
#define kParamZeroLabel "Curve start at 0"
#define kParamZeroHint "Curve start at 0, no negative values."
#define kParamZeroDefault false

#define kParamGenerate "generate"
#define kParamGenerateLabel "Generate"
#define kParamGenerateHint "Generate curve data."

#define kParamCurve "curve"
#define kParamCurveLabel "Curve Data"
#define kParamCurveHint "Generated curve data."

using namespace OFX;

namespace {
// closes the file (if any) and calls sox_quit() on every exit, including exceptions
struct SoxGuard
{
    sox_format_t* audio;
    ~SoxGuard() { if (audio) { sox_close(audio); } sox_quit(); }
};
} // namespace

class AudioCurvePlugin : public ImageEffect
{
public:
    struct CurveData
    {
        int frame;
        double x;
        double y;
    };
    AudioCurvePlugin(OfxImageEffectHandle handle);
    virtual void render(const RenderArguments &args) override final;
    virtual void changedParam(const InstanceChangedArgs &args,
                              const std::string &paramName) override final;
    void generateCurves();
    void setCurveLength();
    bool fileExists(const std::string &str);
    static bool fpsIsValid(double fps) { return fps >= kParamFPSMin && fps <= kParamFPSMax; }

private:
    Clip *_dstClip;
    StringParam *_srcFile;
    DoubleParam *_srcFps;
    Int2DParam *_srcFrames;
    PushButtonParam *_srcGen;
    Double2DParam *_srcCurve;
    Double2DParam *_srcFactor;
    BooleanParam *_srcZero;
};

AudioCurvePlugin::AudioCurvePlugin(OfxImageEffectHandle handle)
: ImageEffect(handle)
 , _dstClip(nullptr)
 , _srcFile(nullptr)
 , _srcFps(nullptr)
 , _srcFrames(nullptr)
 , _srcGen(nullptr)
 , _srcCurve(nullptr)
 , _srcFactor(nullptr)
 , _srcZero(nullptr)
{
    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    assert(_dstClip && _dstClip->getPixelComponents() == ePixelComponentRGBA);

    _srcFile = fetchStringParam(kParamFile);
    _srcFps = fetchDoubleParam(kParamFPS);
    _srcFrames = fetchInt2DParam(kParamFrames);
    _srcGen = fetchPushButtonParam(kParamGenerate);
    _srcCurve = fetchDouble2DParam(kParamCurve);
    _srcFactor = fetchDouble2DParam(kParamFactor);
    _srcZero = fetchBooleanParam(kParamZero);

    assert(_srcFile && _srcFps && _srcFrames &&
           _srcGen && _srcCurve && _srcFactor &&
           _srcZero);
}

/* Override the render */
void AudioCurvePlugin::render(const RenderArguments &args)
{
    // dstclip
    if (!_dstClip) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(_dstClip);

    // get dstclip
    auto_ptr<Image> dstImg(_dstClip->fetchImage(args.time));
    if (!dstImg.get()) {
        throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // renderscale
    checkBadRenderScaleOrField(dstImg, args);

    // get bitdepth
    BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != eBitDepthFloat) {
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get channels
    PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != ePixelComponentRGBA) {
        throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    if (args.renderWindow.x1 < dstBounds.x1 ||
        args.renderWindow.x1 >= dstBounds.x2 ||
        args.renderWindow.y1 < dstBounds.y1 ||
        args.renderWindow.y1 >= dstBounds.y2 ||
        args.renderWindow.x2 <= dstBounds.x1 ||
        args.renderWindow.x2 > dstBounds.x2 ||
        args.renderWindow.y2 <= dstBounds.y1 ||
        args.renderWindow.y2 > dstBounds.y2) {
        throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // write output: transparent black over the render window, addressing dstImg through
    // its bounds and rowBytes, which may differ from the RoD, include padding, or be
    // negative (kOfxImagePropRowBytes).
    for (int y = args.renderWindow.y1; y < args.renderWindow.y2; y++) {
        float* dst = PackedImage::pixelAddress(dstImg->getPixelData(), dstBounds, dstImg->getRowBytes(), 4, args.renderWindow.x1, y);
        for (int x = args.renderWindow.x1; x < args.renderWindow.x2; x++, dst += 4) {
            dst[0] = 0.f;
            dst[1] = 0.f;
            dst[2] = 0.f;
            dst[3] = 0.f;
        }
    }
}

void AudioCurvePlugin::changedParam(const InstanceChangedArgs &/*args*/,
                                    const std::string &paramName)
{
    clearPersistentMessage();
    if (paramName == kParamGenerate) {
        generateCurves();
    } else if (paramName == kParamFile || paramName == kParamFPS) {
        setCurveLength();
    }
}

void AudioCurvePlugin::generateCurves()
{
    std::string filename;
    double fps = 0.0;
    int endFrame = 0;
    int startFrame = 0;
    double factorX = 0.0;
    double factorY = 0.0;
    bool zero = false;
    bool success = false;
    std::vector<CurveData> result;
    double maxX = 0.0;
    double maxY = 0.0;

    _srcZero->getValue(zero);
    _srcFactor->getValue(factorX, factorY);
    _srcFrames->getValue(startFrame, endFrame);
    _srcFps->getValue(fps);
    _srcFile->getValue(filename);

    if (!fileExists(filename) || endFrame == 0 || startFrame == 0 || !fpsIsValid(fps)) {
        setPersistentMessage(Message::eMessageWarning, "", "Bad input");
        _srcCurve->deleteAllKeys();
        _srcCurve->setValue(0.0, 0.0);
        return;
    }

    if (sox_init() != SOX_SUCCESS) {
        setPersistentMessage(Message::eMessageWarning, "", "SoX init failed");
        return;
    }

    sox_format_t *audio = sox_open_read(filename.c_str(),
                                        nullptr,
                                        nullptr,
                                        nullptr);
    const SoxGuard guard = { audio };
    if (audio) {
        size_t blocks;
        size_t block_size;
        static const double block_period = 0.025;
        double start_secs = startFrame<=1?0:startFrame/fps;
        uint64_t seek;
        double period = endFrame/fps;

        // check the channel count (only stereo is supported) and rate before
        // using them: seek % channels would divide by zero if channels == 0
        bool seekOk = false;
        if (audio->signal.channels == 2 && audio->signal.rate >= 1 && audio->signal.rate <= 1e6) { // keeps seek and block_size in range
            seek = start_secs * audio->signal.rate * audio->signal.channels + .5;
            seek -= seek % audio->signal.channels;
            seekOk = sox_seek(audio,
                              seek,
                              SOX_SEEK_SET) == SOX_SUCCESS;
        }
        if (seekOk) {
            block_size = block_period * audio->signal.rate * audio->signal.channels + .5;
            block_size -= block_size % audio->signal.channels;
            std::vector<sox_sample_t> buf(block_size);
            int lastFrame = -1;

            // period is the absolute end time, while blocks are counted from
            // start_secs: stop at the end time, not period seconds after the
            // start (#51). A short read (end of file) is still processed.
            for (blocks = 0; start_secs + blocks * block_period < period; ++blocks)
            {
                size_t nread = sox_read(audio, buf.data(), block_size);
                nread -= nread % audio->signal.channels; // whole sample frames only
                if (nread == 0) {
                    break;
                }
                double left = 0.0;
                double right = 0.0;
                size_t i;
                for (i = 0; i < nread; ++i) {
                    SOX_SAMPLE_LOCALS;
                    double sample = SOX_SAMPLE_TO_FLOAT_64BIT(buf[i],);
                    // the block's peak: max |sample| with zero, else the signed sample of largest magnitude
                    double& peak = (i & 1) ? right : left;
                    if (fabs(sample) > fabs(peak)) { peak = zero ? fabs(sample) : sample; }
                }

                if (fabs(left)>maxX) { maxX = fabs(left); }
                if (fabs(right)>maxY) { maxY = fabs(right); }

                int frame = 0;
                double secs = start_secs + blocks * block_period;
                frame = (secs*fps)+1;

                if (frame>lastFrame) {
                    result.push_back({frame, left, right});
                    if (!success) { success = true; }
                }
                lastFrame = frame;
                if (nread < block_size) {
                    break; // end of file
                }
            }
        }
    }

    if (!success) {
        setPersistentMessage(Message::eMessageWarning, "", "Failed to read the file");
    } else {
        _srcCurve->deleteAllKeys();
        for (size_t i=0;i<result.size();++i) {
            CurveData curve = result.at(i);
            // normalize by the max, unless it is zero (e.g. silent audio),
            // which would give NaN/inf keyframes
            _srcCurve->setValueAtTime(curve.frame,
                                      maxX > 0. ? curve.x*(factorX/maxX) : curve.x*factorX,
                                      maxY > 0. ? curve.y*(factorY/maxY) : curve.y*factorY);
        }
    }
}

void AudioCurvePlugin::setCurveLength()
{
    std::string filename;
    double fps = 0.0;
    int startFrame = 0;
    int endFrame = 0;

    _srcFps->getValue(fps);
    _srcFile->getValue(filename);
    _srcFrames->getValue(startFrame, endFrame);

    if (!fileExists(filename)) {
        setPersistentMessage(Message::eMessageWarning, "", "File does not exist");
        return;
    }
    if (!fpsIsValid(fps)) {
        setPersistentMessage(Message::eMessageWarning, "", "Bad frame rate");
        return;
    }

    if (sox_init() != SOX_SUCCESS) {
        setPersistentMessage(Message::eMessageWarning, "", "SoX init failed");
        return;
    }

    sox_format_t *audio = sox_open_read(filename.c_str(),
                                        nullptr,
                                        nullptr,
                                        nullptr);
    const SoxGuard guard = { audio };
    if (audio) {
        // channels == 0 would divide by zero, and the length may be unknown (e.g. streams)
        if (audio->signal.channels == 0 || !(audio->signal.rate >= 1 && audio->signal.rate <= 1e6) ||
            audio->signal.length == SOX_UNKNOWN_LEN) {
            setPersistentMessage(Message::eMessageWarning, "", "Unknown audio length");
        } else {
            uint64_t ws = audio->signal.length/audio->signal.channels;
            // ws and rate come from the file header: clamp to int
            _srcFrames->setValue(startFrame, static_cast<int>(std::min(std::floor((double)ws/audio->signal.rate) * fps, double(INT_MAX))));
        }
    } else {
        setPersistentMessage(Message::eMessageWarning, "", "Failed to read the file");
    }
}

bool AudioCurvePlugin::fileExists(const std::string &str)
{
    struct stat st;
    return (stat(str.c_str(), &st) == 0);
}

mDeclarePluginFactory(AudioCurvePluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void AudioCurvePluginFactory::describe(ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add the supported contexts
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // add other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void AudioCurvePluginFactory::describeInContext(ImageEffectDescriptor &desc,
                                                ContextEnum /*context*/)
{
    // there has to be an input clip, even for generators
    ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->setOptional(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam("Controls");
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamFile);
        param->setLabel(kParamFileLabel);
        param->setHint(kParamFileHint);
        param->setStringType(eStringTypeFilePath);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        DoubleParamDescriptor* param = desc.defineDoubleParam(kParamFPS);
        param->setLabel(kParamFPSLabel);
        param->setHint(kParamFPSHint);
        param->setDefault(kParamFPSDefault);
        param->setRange(kParamFPSMin, kParamFPSMax); // a negative fps gave a negative seek position
        param->setDisplayRange(kParamFPSMin, 120.); // setRange() also resets the display range
        param->setAnimates(false);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        Int2DParamDescriptor* param = desc.defineInt2DParam(kParamFrames);
        param->setLabel(kParamFramesLabel);
        param->setHint(kParamFramesHint);
        param->setDefault(1, kParamFramesDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamZero);
        param->setLabel(kParamZeroLabel);
        param->setHint(kParamZeroHint);
        param->setDefault(kParamZeroDefault);
        param->setAnimates(false);
        param->setLayoutHint(eLayoutHintNoNewLine, 1);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        Double2DParamDescriptor* param = desc.defineDouble2DParam(kParamFactor);
        param->setLabel(kParamFactorLabel);
        param->setHint(kParamFactorHint);
        param->setDefault(kParamFactorDefault, kParamFactorDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        Double2DParamDescriptor* param = desc.defineDouble2DParam(kParamCurve);
        param->setLabel(kParamCurveLabel);
        param->setHint(kParamCurveHint);
        param->setAnimates(true);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        PushButtonParamDescriptor* param = desc.definePushButtonParam(kParamGenerate);
        param->setLabel(kParamGenerateLabel);
        param->setHint(kParamGenerateHint);
        if (page) {
            page->addChild(*param);
        }
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* AudioCurvePluginFactory::createInstance(OfxImageEffectHandle handle,
                                                     ContextEnum /*context*/)
{
    return new AudioCurvePlugin(handle);
}

static AudioCurvePluginFactory p(kPluginIdentifier,
                                 kPluginVersionMajor,
                                 kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
