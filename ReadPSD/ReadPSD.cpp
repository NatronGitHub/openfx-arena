/*

openfx-arena - https://github.com/olear/openfx-arena

Copyright (c) 2015, Ole-André Rodlie <olear@fxarena.net>
Copyright (c) 2015, FxArena DA <mail@fxarena.net>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Neither the name of FxArena DA nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ReadPSD.h"
#include <iostream>
#include <stdint.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxNatron.h"
#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadPSD"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadPSD"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 1

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

#define kParamLayer "layer"
#define kParamLayerLabel "Image layer"
#define kParamLayerHint "Layer"
#define kParamLayerDefault 0

#define OFX_READ_OIIO_NEWMENU

#ifndef OFX_READ_OIIO_NEWMENU
#define kParamFirstChannel "firstChannel"
#define kParamFirstChannelLabel "First Channel"
#define kParamFirstChannelHint "Channel from the input file corresponding to the first component."
#endif

#ifdef OFX_READ_OIIO_NEWMENU

#define kParamRChannel "rChannel"
#define kParamRChannelLabel "R Channel"
#define kParamRChannelHint "Channel from the input file corresponding to the red component."

#define kParamGChannel "gChannel"
#define kParamGChannelLabel "G Channel"
#define kParamGChannelHint "Channel from the input file corresponding to the green component."

#define kParamBChannel "bChannel"
#define kParamBChannelLabel "B Channel"
#define kParamBChannelHint "Channel from the input file corresponding to the blue component."

#define kParamAChannel "aChannel"
#define kParamAChannelLabel "A Channel"
#define kParamAChannelHint "Channel from the input file corresponding to the alpha component."

#define kParamRChannelName "rChannelIndex"
#define kParamGChannelName "gChannelIndex"
#define kParamBChannelName "bChannelIndex"
#define kParamAChannelName "aChannelIndex"

// number of channels for hosts that don't support modifying choice menus (e.g. Nuke)
#define kDefaultChannelCount 16

// Channels 0 and 1 are reserved for 0 and 1 constants
#define kXChannelFirst 2

#endif // OFX_READ_OIIO_NEWMENU


static bool gHostIsNatron   = false;

class ReadPSDPlugin : public GenericReaderPlugin
{
public:
    ReadPSDPlugin(OfxImageEffectHandle handle);
    virtual ~ReadPSDPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    virtual void restoreState(const std::string& filename) OVERRIDE FINAL;
    int getImageLayers(const std::string& filename);
    void genLayerMenu(const std::string& filename);
    OFX::ChoiceParam *_layer;
#ifdef OFX_READ_OIIO_NEWMENU
    OFX::ChoiceParam *_rChannel;
    OFX::ChoiceParam *_gChannel;
    OFX::ChoiceParam *_bChannel;
    OFX::ChoiceParam *_aChannel;
    OFX::StringParam *_rChannelName;
    OFX::StringParam *_gChannelName;
    OFX::StringParam *_bChannelName;
    OFX::StringParam *_aChannelName;
#else
    OFX::IntParam *_firstChannel;
#endif
};

ReadPSDPlugin::ReadPSDPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
,_layer(0)
#ifdef OFX_READ_OIIO_NEWMENU
, _rChannel(0)
, _gChannel(0)
, _bChannel(0)
, _aChannel(0)
#else
, _firstChannel(0)
#endif
{
    Magick::InitializeMagick(NULL);
    _layer = fetchChoiceParam(kParamLayer);
    assert(_layer);
#ifdef OFX_READ_OIIO_NEWMENU
    _rChannel = fetchChoiceParam(kParamRChannel);
    _gChannel = fetchChoiceParam(kParamGChannel);
    _bChannel = fetchChoiceParam(kParamBChannel);
    _aChannel = fetchChoiceParam(kParamAChannel);
    _rChannelName = fetchStringParam(kParamRChannelName);
    _gChannelName = fetchStringParam(kParamGChannelName);
    _bChannelName = fetchStringParam(kParamBChannelName);
    _aChannelName = fetchStringParam(kParamAChannelName);
    assert(_outputComponents && _rChannel && _gChannel && _bChannel && _aChannel &&
           _rChannelName && _bChannelName && _gChannelName && _aChannelName);
#else
    _firstChannel = fetchIntParam(kParamFirstChannel);
    assert(_outputComponents && _firstChannel);
#endif
}

ReadPSDPlugin::~ReadPSDPlugin()
{
}

int ReadPSDPlugin::getImageLayers(const std::string &filename)
{
    int layers = 0;
    int max = 999;
    Magick::Image image;
    while (layers<max) {
        std::ostringstream layer;
        layer << filename;
        layer << "[" << layers << "]";
        try {
            image.read(layer.str());
        }
        catch(Magick::Exception) {
            break;
        }
        layers++;
    }
    if (layers>0)
        layers--;
    return layers;
}

void ReadPSDPlugin::decode(const std::string& filename,
                      OfxTime time,
                      const OfxRectI& /*renderWindow*/,
                      float *pixelData,
                      const OfxRectI& bounds,
                      OFX::PixelComponentEnum pixelComponents,
                      int pixelComponentCount,
                      int /*rowBytes*/)
{
    int layer = 0;
    _layer->getValueAtTime(time, layer);
    Magick::Image image;
    image.backgroundColor("none");
    std::ostringstream file;
    file << filename.c_str();
    file << "[";
    file << layer;
    file << "]";
    image.read(file.str());
    if (image.columns()>0 && image.rows()>0) {
        Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
        container.composite(image,0,0,Magick::OverCompositeOp);
        container.flip();
        container.write(0,0,bounds.x2,bounds.y2,"RGBA",Magick::FloatPixel,pixelData);
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

bool ReadPSDPlugin::getFrameBounds(const std::string& filename,
                              OfxTime time,
                              OfxRectI *bounds,
                              double *par,
                              std::string */*error*/)
{
    int layer = 0;
    _layer->getValueAtTime(time, layer);
    Magick::Image image;
    std::ostringstream file;
    file << filename.c_str();
    file << "[";
    file << layer;
    file << "]";
    image.read(file.str());
    if (image.columns()>0 && image.rows()>0) {
        bounds->x1 = 0;
        bounds->x2 = image.columns();
        bounds->y1 = 0;
        bounds->y2 = image.rows();
        *par = 1.0;
        return true;
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    return false;
}

void ReadPSDPlugin::genLayerMenu(const std::string& filename)
{
    if (gHostIsNatron) {
        _layer->resetOptions();
        int layers = getImageLayers(filename)+1;
        for (int i = 0; i < layers; ++i) {
            std::ostringstream layer;
            layer << i;
            _layer->appendOption(layer.str());
        }
    }
}

void ReadPSDPlugin::restoreState(const std::string& filename)
{
    genLayerMenu(filename);
}

void ReadPSDPlugin::onInputFileChanged(const std::string& newFile,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int *componentCount)
{
    assert(premult && components);
# ifdef OFX_IO_USING_OCIO
    Magick::Image image;
    image.read(newFile);
    switch(image.colorSpace()) {
    case Magick::RGBColorspace:
        _ocio->setInputColorspace("sRGB");
        break;
    case Magick::sRGBColorspace:
        _ocio->setInputColorspace("sRGB");
        break;
    case Magick::scRGBColorspace:
        _ocio->setInputColorspace("sRGB");
        break;
    }
# endif // OFX_IO_USING_OCIO
    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageOpaque;
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadPSDPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void ReadPSDPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"psd", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(92);
    #endif

    std::string magickV = MagickCore::GetMagickVersion(NULL);
    std::string delegates = MagickCore::GetMagickDelegates();
    desc.setPluginDescription("Read PSD image format.\n\nWritten by Ole-André Rodlie <olear@fxarena.net>\n\n Powered by "+magickV+"\n\nFeatures: "+delegates);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadPSDPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (OFX::getImageEffectHostDescription()->hostName == kNatronOfxHostName);
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamLayer);
        param->setLabel(kParamLayerLabel);
        param->setHint(kParamLayerHint);
        param->appendOption("0");
        param->appendOption("1");
        page->addChild(*param);
    }
#ifndef OFX_READ_OIIO_NEWMENU
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamFirstChannel);
        param->setLabel(kParamFirstChannelLabel, kParamFirstChannelLabel, kParamFirstChannelLabel);
        param->setHint(kParamFirstChannelHint);
        page->addChild(*param);
    }
#endif


#ifdef OFX_READ_OIIO_NEWMENU
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamRChannel);
        param->setLabel(kParamRChannelLabel);
        param->setHint(kParamRChannelHint);
        param->appendOption("0");
        param->appendOption("1");
        param->setAnimates(true);
        param->setIsPersistant(false); //don't save, we will restore it using the StringParams holding the index
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamGChannel);
        param->setLabel(kParamGChannelLabel);
        param->setHint(kParamGChannelHint);
        param->appendOption("0");
        param->appendOption("1");
        param->setAnimates(true);
        param->setIsPersistant(false); //don't save, we will restore it using the StringParams holding the index
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamBChannel);
        param->setLabel(kParamBChannelLabel);
        param->setHint(kParamBChannelHint);
        param->appendOption("0");
        param->appendOption("1");
        param->setAnimates(true);
        param->setIsPersistant(false); //don't save, we will restore it using the StringParams holding the index
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamAChannel);
        param->setLabel(kParamAChannelLabel);
        param->setHint(kParamAChannelHint);
        param->appendOption("0");
        param->appendOption("1");
        param->setAnimates(true);
        param->setDefault(1); // opaque by default
        param->setIsPersistant(false); //don't save, we will restore it using the StringParams holding the index
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamRChannelName);
        param->setLabel(kParamRChannelLabel);
        param->setHint(kParamRChannelHint);
        param->setAnimates(false);
        param->setIsSecret(true); // never meant to be visible
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamGChannelName);
        param->setLabel(kParamGChannelLabel);
        param->setHint(kParamGChannelHint);
        param->setAnimates(false);
        param->setIsSecret(true); // never meant to be visible
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamBChannelName);
        param->setLabel(kParamBChannelLabel);
        param->setHint(kParamBChannelHint);
        param->setAnimates(false);
        param->setIsSecret(true); // never meant to be visible
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamAChannelName);
        param->setLabel(kParamAChannelLabel);
        param->setHint(kParamAChannelHint);
        param->setAnimates(false);
        param->setIsSecret(true); // never meant to be visible
        page->addChild(*param);
    }
#endif
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadPSDPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadPSDPlugin* ret =  new ReadPSDPlugin(handle);
    ret->restoreStateFromParameters();
    return ret;
}

void getReadPSDPluginID(OFX::PluginFactoryArray &ids)
{
    static ReadPSDPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
