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

#include <stdint.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#define kPluginName "AudioCurveOFX"
#define kPluginIdentifier "net.fxarena.openfx.AudioCurve"
#ifndef kPluginVersionMajor
#define kPluginVersionMajor 0
#endif
#ifndef kPluginVersionMinor
#define kPluginVersionMinor 9
#endif
#define kPluginGrouping "Other"
#define kPluginDescription "Generate curve data from audio files using libSoX."

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

#define kParamSmooth "smooth"
#define kParamSmoothLabel "Curve Smooth"
#define kParamSmoothHint "Smooth the curves."
#define kParamSmoothDefault false

#define kParamGenerate "generate"
#define kParamGenerateLabel "Generate"
#define kParamGenerateHint "Generate curve data."

#define kParamCurve "curve"
#define kParamCurveLabel "Curve Data"
#define kParamCurveHint "Generated curve data."

using namespace OFX;

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

private:
    Clip *_dstClip;
    StringParam *_srcFile;
    DoubleParam *_srcFps;
    Int2DParam *_srcFrames;
    PushButtonParam *_srcGen;
    Double2DParam *_srcCurve;
    Double2DParam *_srcFactor;
    BooleanParam *_srcZero;
    BooleanParam *_srcSmooth;
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
 , _srcSmooth(nullptr)
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
    _srcSmooth = fetchBooleanParam(kParamSmooth);

    assert(_srcFile && _srcFps && _srcFrames &&
           _srcGen && _srcCurve && _srcFactor &&
           _srcZero && _srcSmooth);
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
    OfxRectI dstRod = dstImg->getRegionOfDefinition();
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

    // get options
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;

    // write output
    float* pixelData = (float*)dstImg->getPixelData();
    int offset = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixelData[offset + 0] = 0.f;
            pixelData[offset + 1] = 0.f;
            pixelData[offset + 2] = 0.f;
            pixelData[offset + 3] = 0.f;
            offset += 4;
        }
    }
    pixelData = nullptr;
}

void AudioCurvePlugin::changedParam(const InstanceChangedArgs &args,
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
    bool smooth = false;
    bool zero = false;

    _srcZero->getValue(zero);
    _srcSmooth->getValue(smooth);
    _srcFactor->getValue(factorX, factorY);
    _srcFrames->getValue(startFrame, endFrame);
    _srcFps->getValue(fps);
    _srcFile->getValue(filename);

    if (!fileExists(filename) || endFrame == 0) {
        setPersistentMessage(Message::eMessageWarning, "", "File does not exist");
        _srcCurve->deleteAllKeys();
        _srcCurve->setValue(0.0, 0.0);
        return;
    }

    sox_format_t * audio;
    sox_sample_t * buf;
    size_t blocks, block_size;
    static const double block_period = 0.025;
    double start_secs = startFrame<=1?0:startFrame/fps;
    uint64_t seek;
    bool success = false;
    double period = endFrame/fps;
    std::vector<CurveData> result;
    double maxX = 0.0;
    double maxY = 0.0;

    assert(sox_init() == SOX_SUCCESS);
    audio = sox_open_read(filename.c_str(),
                          nullptr,
                          nullptr,
                          nullptr);
    if (audio) {
        seek = start_secs * audio->signal.rate * audio->signal.channels + .5;
        seek -= seek % audio->signal.channels;
        if (sox_seek(audio,
                     seek,
                     SOX_SEEK_SET) == SOX_SUCCESS &&
            audio->signal.channels == 2)
        {
            block_size = block_period * audio->signal.rate * audio->signal.channels + .5;
            block_size -= block_size % audio->signal.channels;
            assert(buf = (sox_sample_t*)malloc(sizeof(sox_sample_t) * block_size));

            success = true;
            int lastFrame = -1;
            for (blocks = 0; sox_read(audio,
                                      buf,
                                      block_size) == block_size &&
                 blocks * block_period < period; ++blocks)
            {
                double left = 0.0;
                double right = 0.0;
                size_t i;
                for (i = 0; i < block_size; ++i) {
                    SOX_SAMPLE_LOCALS;
                    double sample = SOX_SAMPLE_TO_FLOAT_64BIT(buf[i],);
                    if (!zero) {
                        if (i & 1) { right = sample; }
                        else { left = sample; }
                    } else {
                        if (i & 1) { right = std::max(right, fabs(sample)); }
                        else { left = std::max(left, fabs(sample)); }
                    }
                }

                if (left>maxX) { maxX = left; }
                if (right>maxY) { maxY = right; }

                int frame = 0;
                double secs = start_secs + blocks * block_period;
                if (smooth) { frame = ((int)secs*fps)+1; }
                else { frame = (secs*fps)+1; }

                if (frame>lastFrame) {
                    result.push_back({frame, left, right});
                }
                lastFrame = frame;
            }
        }
        sox_close(audio);
    }

    free(buf);
    sox_quit();

    if (!success) {
        setPersistentMessage(Message::eMessageWarning, "", "Failed to read the file");
    } else {
        _srcCurve->deleteAllKeys();
        for (int i=0;i<result.size();++i) {
            _srcCurve->setValueAtTime(result.at(i).frame,
                                      result.at(i).x*(factorX/maxX),
                                      result.at(i).y*(factorY/maxY));
        }
    }
}

void AudioCurvePlugin::setCurveLength()
{
    std::string filename;
    double fps = 0.0;
    int dur = 0;
    int startFrame = 0;
    int endFrame = 0;

    _srcFps->getValue(fps);
    _srcFile->getValue(filename);
    _srcFrames->getValue(startFrame, endFrame);

    if (!fileExists(filename)) {
        setPersistentMessage(Message::eMessageWarning, "", "File does not exist");
        return;
    }

    assert(sox_init() == SOX_SUCCESS);
    sox_format_t *audio = sox_open_read(filename.c_str(),
                                        nullptr,
                                        nullptr,
                                        nullptr);
    if (audio) {
        uint64_t ws = audio->signal.length/audio->signal.channels;
        dur = audio->signal.length?(double)ws/audio->signal.rate:0;
        _srcFrames->setValue(startFrame, dur*fps);
        sox_close(audio);
    } else {
        setPersistentMessage(Message::eMessageWarning, "", "Failed to read the file");
    }
    sox_quit();
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
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamSmooth);
        param->setLabel(kParamSmoothLabel);
        param->setHint(kParamSmoothHint);
        param->setDefault(kParamSmoothDefault);
        param->setAnimates(false);
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
