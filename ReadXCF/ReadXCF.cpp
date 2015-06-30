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

#include "ReadXCF.h"
#include <iostream>
#include <stdint.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadXCF"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadXCF"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 2

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar true

class ReadXCFPlugin : public GenericReaderPlugin
{
public:
    ReadXCFPlugin(OfxImageEffectHandle handle);
    virtual ~ReadXCFPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds,
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
        decodePlane(filename, time, renderWindow, pixelData, bounds, pixelComponents, pixelComponentCount, rawComps, rowBytes);
    }
    virtual void getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents) OVERRIDE FINAL;
    virtual void decodePlane(const std::string& filename, OfxTime time, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, const std::string& rawComponents, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error) OVERRIDE FINAL;
    virtual void restoreState(const std::string& filename) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    std::string _filename;
    std::vector<Magick::Image> _xcf;
};

ReadXCFPlugin::ReadXCFPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles,
#ifdef OFX_EXTENSIONS_NUKE
(OFX::getImageEffectHostDescription() && OFX::getImageEffectHostDescription()->isMultiPlanar) ? kIsMultiPlanar : false
#else
false
#endif
)
{
    Magick::InitializeMagick(NULL);
    assert(_outputComponents);
}

ReadXCFPlugin::~ReadXCFPlugin()
{
}

void ReadXCFPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    #ifdef DEBUG
    std::cout << "getClipComponents ..." << std::endl;
    #endif
    assert(isMultiPlanar());
    clipComponents.addClipComponents(*_outputClip, getOutputComponents());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    //if (_xcf.size()>0) { // in xcf all images are layers, in psd first is complete image
        for (size_t i = 0; i < _xcf.size(); i++) {
            //if (i!=0) {
                std::ostringstream layerName;
                layerName << _xcf[i].label();
                if (layerName.str().empty())
                    layerName << "XCF Layer #" << i;
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
            //}
        }
    //}
}

void ReadXCFPlugin::decodePlane(const std::string& /*filename*/, OfxTime /*time*/, const OfxRectI& /*renderWindow*/, float *pixelData, const OfxRectI& bounds,
                                 OFX::PixelComponentEnum /*pixelComponents*/, int /*pixelComponentCount*/, const std::string& rawComponents, int /*rowBytes*/)
{
    #ifdef DEBUG
    std::cout << "decodePlane ..." << std::endl;
    #endif
    Magick::Image container(Magick::Geometry(bounds.x2,bounds.y2),Magick::Color("rgba(0,0,0,0)"));
    std::string layerName;
    std::string::size_type prev_pos = 0, pos = 0;
    while( (pos = rawComponents.find('_', pos)) != std::string::npos ) { // TODO meh, find a better solution...
        std::string substring( rawComponents.substr(prev_pos, pos-prev_pos) );
        if (substring!="NatronOfxImageComponentsPlane" && substring!="Channel" && substring!="R" && substring!="G" && substring!="B" && substring!="A")
            layerName = substring;
        prev_pos = ++pos;
    }
    if (!layerName.empty()) {
        for (size_t i = 0; i < _xcf.size(); i++) {
            if (_xcf[i].label()==layerName) {
                #ifdef DEBUG
                std::cout << "found layer! " << layerName << std::endl;
                #endif
                container.composite(_xcf[i],_xcf[i].page().xOff(),_xcf[i].page().yOff(),Magick::OverCompositeOp);
                break;
            }
            std::ostringstream xcfLayer;
            xcfLayer << "XCF Layer #" << i;
            if (xcfLayer.str()==layerName) {
                #ifdef DEBUG
                std::cout << "found layer! " << layerName << std::endl;
                #endif
                container.composite(_xcf[i],_xcf[i].page().xOff(),_xcf[i].page().yOff(),Magick::OverCompositeOp);
                break;
            }
        }
    }
    else // fail, show layer 0
        container.composite(_xcf[0],0,0,Magick::OverCompositeOp);
    container.flip();
    container.write(0,0,bounds.x2,bounds.y2,"RGBA",Magick::FloatPixel,pixelData);
}

bool ReadXCFPlugin::getFrameBounds(const std::string& /*filename*/,
                              OfxTime /*time*/,
                              OfxRectI *bounds,
                              double *par,
                              std::string */*error*/)
{
    #ifdef DEBUG
    std::cout << "getFrameBounds ..." << std::endl;
    #endif
    if (_xcf[0].columns()>0 && _xcf[0].rows()>0) {
        bounds->x1 = 0;
        bounds->x2 = _xcf[0].columns();
        bounds->y1 = 0;
        bounds->y2 = _xcf[0].rows();
        *par = 1.0;
    }
    return true;
}

void ReadXCFPlugin::restoreState(const std::string& filename)
{
    #ifdef DEBUG
    std::cout << "restoreState ..." << std::endl;
    #endif
    _xcf.clear();
    try {
        Magick::readImages(&_xcf, filename);
    }
    catch(Magick::Exception) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    if (_xcf[0].columns()>0 && _xcf[0].rows()>0)
        _filename = filename;
    else {
        _xcf.clear();
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

void ReadXCFPlugin::onInputFileChanged(const std::string& newFile,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int */*componentCount*/)
{
    #ifdef DEBUG
    std::cout << "onInputFileChanged ..." << std::endl;
    #endif
    assert(premult && components);
    if (newFile!=_filename)
        restoreState(newFile);
    # ifdef OFX_IO_USING_OCIO
    switch(_xcf[0].colorSpace()) {
    case Magick::RGBColorspace:
        _ocio->setInputColorspace("sRGB");
        break;
    case Magick::sRGBColorspace:
        _ocio->setInputColorspace("sRGB");
        break;
    case Magick::scRGBColorspace:
        _ocio->setInputColorspace("sRGB");
        break;
    case Magick::Rec709LumaColorspace:
        _ocio->setInputColorspace("Rec709");
        break;
    case Magick::Rec709YCbCrColorspace:
        _ocio->setInputColorspace("Rec709");
        break;
    default:
        _ocio->setInputColorspace("Linear");
        break;
    }
    # endif // OFX_IO_USING_OCIO
    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageOpaque;
}

using namespace OFX;

mDeclareReaderPluginFactory(ReadXCFPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void ReadXCFPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, kIsMultiPlanar);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"xcf", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(50);
    #endif

    std::string magickV = MagickCore::GetMagickVersion(NULL);
    desc.setPluginDescription("Read GIMP/XCF image format.\n\nWritten by Ole-André Rodlie <olear@fxarena.net>\n\nPowered by "+magickV);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadXCFPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ReadXCFPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ReadXCFPlugin* ret =  new ReadXCFPlugin(handle);
    ret->restoreStateFromParameters();
    return ret;
}

void getReadXCFPluginID(OFX::PluginFactoryArray &ids)
{
    static ReadXCFPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
