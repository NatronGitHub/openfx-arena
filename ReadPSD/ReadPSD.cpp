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
#include <string>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include <lcms2.h>
#include <dirent.h>

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadPSD"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadPSD"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 8

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar true

#define kParamICCIn "iccIn"
#define kParamICCInLabel "ICC input profile"
#define kParamICCInHint "ICC input profile"

#define kParamICCOut "iccOut"
#define kParamICCOutLabel "ICC output profile"
#define kParamICCOutHint "ICC output profile"

void _getProFiles(std::vector<std::string> &files, bool desc, std::string filter, int colorspace) {
    std::vector<std::string> paths;
    paths.push_back("/usr/share/color/icc/");
    paths.push_back("\\Windows\\system32\\spool\\drivers\\color\\");
    paths.push_back("/Library/ColorSync/Profiles/");
    for (unsigned int i = 0; i < paths.size(); i++) {
        DIR *dp;
        struct dirent *dirp;
        if ((dp=opendir(paths[i].c_str())) != NULL) {
            while ((dirp=readdir(dp)) != NULL) {
                std::string proFile = dirp->d_name;
                std::ostringstream profileDesc;
                    cmsHPROFILE lcmsProfile;
                    char buffer[500];
                    std::ostringstream path;
                    int iccColorspace = 0;
                    path << paths[i] << proFile;
                    lcmsProfile = cmsOpenProfileFromFile(path.str().c_str(), "r");
                    if (lcmsProfile) {
                        cmsGetProfileInfoASCII(lcmsProfile, cmsInfoDescription, "en", "US", buffer, 500);
                        profileDesc << buffer;
                        if(cmsGetColorSpace(lcmsProfile) == cmsSigRgbData)
                            iccColorspace = 1;
                        if(cmsGetColorSpace(lcmsProfile) == cmsSigCmykData)
                            iccColorspace = 2;
                        if(cmsGetColorSpace(lcmsProfile) == cmsSigGrayData)
                            iccColorspace = 3;
                    }
                    cmsCloseProfile(lcmsProfile);
                    if (!profileDesc.str().empty() && (colorspace==iccColorspace||colorspace>3)) {
                        if (!filter.empty()) {
                            if (profileDesc.str()==filter) {
                                files.push_back(path.str());
                                break;
                            }
                        }
                        else {
                            if (desc)
                                files.push_back(profileDesc.str());
                            else
                                files.push_back(proFile);
                        }
                    }
            }
        }
        closedir(dp);
    }
}

class ReadPSDPlugin : public GenericReaderPlugin
{
public:
    ReadPSDPlugin(OfxImageEffectHandle handle);
    virtual ~ReadPSDPlugin();
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
    std::vector<Magick::Image> _psd;
    OFX::ChoiceParam *_iccIn;
    OFX::ChoiceParam *_iccOut;
    bool _hasLCMS;
};

ReadPSDPlugin::ReadPSDPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles,
#ifdef OFX_EXTENSIONS_NUKE
(OFX::getImageEffectHostDescription() && OFX::getImageEffectHostDescription()->isMultiPlanar) ? kIsMultiPlanar : false
#else
false
#endif
)
,_hasLCMS(false)
{
    Magick::InitializeMagick(NULL);

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("lcms") != std::string::npos)
        _hasLCMS = true;

    _iccIn = fetchChoiceParam(kParamICCIn);
    _iccOut = fetchChoiceParam(kParamICCOut);
    assert(_outputComponents && _iccIn && _iccOut);
}

ReadPSDPlugin::~ReadPSDPlugin()
{
}

void ReadPSDPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    #ifdef DEBUG
    std::cout << "getClipComponents ..." << std::endl;
    #endif
    assert(isMultiPlanar());
    clipComponents.addClipComponents(*_outputClip, getOutputComponents());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (_psd.size()>0) {
        int startLayer = 0;
        if (_psd[0].format()=="Adobe Photoshop bitmap")
            startLayer++;
        for (int i = startLayer; i < (int)_psd.size(); i++) {
            std::ostringstream layerName;
            layerName << _psd[i].label();
            if (layerName.str().empty())
                layerName << "Image Layer #" << i; // add a label if empty
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
        }
    }
}

void ReadPSDPlugin::decodePlane(const std::string& /*filename*/, OfxTime time, const OfxRectI& /*renderWindow*/, float *pixelData, const OfxRectI& bounds,
                                 OFX::PixelComponentEnum /*pixelComponents*/, int /*pixelComponentCount*/, const std::string& rawComponents, int /*rowBytes*/)
{
    #ifdef DEBUG
    std::cout << "decodePlane ..." << std::endl;
    #endif
    int offsetX = 0;
    int offsetY = 0;
    int layer = 0;
    int width = bounds.x2;
    int height = bounds.y2;
    std::string layerName;
    int iccProfileInID = 0;
    std::string iccProfileIn;
    int iccProfileOutID = 0;
    std::string iccProfileOut;
    std::vector<std::string> layerChannels = OFX::mapPixelComponentCustomToLayerChannels(rawComponents);
    int numChannels = layerChannels.size();
    _iccIn->getValueAtTime(time, iccProfileInID);
    _iccIn->getOption(iccProfileInID, iccProfileIn);
    _iccOut->getValueAtTime(time, iccProfileOutID);
    _iccOut->getOption(iccProfileOutID, iccProfileOut);

    // Get layer
    if (numChannels==5) // layer+R+G+B+A
        layerName=layerChannels[0];
    if (!layerName.empty()) {
        for (size_t i = 0; i < _psd.size(); i++) {
            bool foundLayer = false;
            std::ostringstream psdLayer;
            psdLayer << "Image Layer #" << i;
            if (_psd[i].label()==layerName)
                foundLayer = true;
            if (psdLayer.str()==layerName && !foundLayer)
                foundLayer = true;
            if (foundLayer) {
                if ((int)_psd[i].columns()!=bounds.x2)
                    offsetX = _psd[i].page().xOff();
                if ((int)_psd[i].rows()!=bounds.y2)
                    offsetY = _psd[i].page().yOff();
                layer = i;
                break;
            }
        }
    }

    // Get image
    Magick::Image image;
    image = _psd[layer];

    // ICC input
    if (!iccProfileIn.empty() && iccProfileIn.find("None") == std::string::npos) {
        std::vector<std::string> profile;
        _getProFiles(profile, false, iccProfileIn,4);
        if (!profile[0].empty()) {
            if (!_hasLCMS)
                setPersistentMessage(OFX::Message::eMessageError, "", "LCMS support missing, unable to convert");
            else {
                Magick::Blob iccBlob;
                Magick::Image iccExtract(profile[0]);
                iccExtract.write(&iccBlob);
                if (iccBlob.length()>0)
                    image.profile("ICC",iccBlob);
            }
        }
    }

    // ICC output
    if (!iccProfileOut.empty() && iccProfileOut.find("None") == std::string::npos) {
        std::vector<std::string> profile;
        _getProFiles(profile, false, iccProfileOut,1);
        if (!profile[0].empty()) {
            if (!_hasLCMS)
                setPersistentMessage(OFX::Message::eMessageError, "", "LCMS support missing, unable to convert");
            else {
                Magick::Blob iccBlob;
                Magick::Image iccExtract(profile[0]);
                iccExtract.write(&iccBlob);
                if (iccBlob.length()>0)
                    image.profile("ICC",iccBlob);
            }
        }
    }

    // Return image
    Magick::Image container(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    container.composite(image,offsetX,offsetY,Magick::OverCompositeOp);
    container.flip();
    container.write(0,0,width,height,"RGBA",Magick::FloatPixel,pixelData);
}

bool ReadPSDPlugin::getFrameBounds(const std::string& /*filename*/,
                              OfxTime /*time*/,
                              OfxRectI *bounds,
                              double *par,
                              std::string */*error*/)
{
    #ifdef DEBUG
    std::cout << "getFrameBounds ..." << std::endl;
    #endif
    if (_psd[0].columns()>0 && _psd[0].rows()>0) {
        bounds->x1 = 0;
        bounds->x2 = _psd[0].columns();
        bounds->y1 = 0;
        bounds->y2 = _psd[0].rows();
        *par = 1.0;
    }
    #ifdef DEBUG
    std::cout << "bounds " << bounds->x2 << "x" << bounds->y2 << std::endl;
    #endif
    return true;
}

void ReadPSDPlugin::restoreState(const std::string& filename)
{
    #ifdef DEBUG
    std::cout << "restoreState ..." << std::endl;
    #endif
    _psd.clear();
    try {
        Magick::readImages(&_psd, filename);
    }
    catch(Magick::Exception) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    if (_psd[0].columns()>0 && _psd[0].rows()>0) {
        _filename = filename;
    }
    else {
        _psd.clear();
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

void ReadPSDPlugin::onInputFileChanged(const std::string& newFile,
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
    switch(_psd[0].colorSpace()) {
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

mDeclareReaderPluginFactory(ReadPSDPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void ReadPSDPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, kIsMultiPlanar);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"psd", "xcf", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(92);
    #endif

    std::string magickV = MagickCore::GetMagickVersion(NULL);
    desc.setPluginDescription("Read Photoshop/GIMP/Cinepaint image formats.\n\nWritten by Ole-André Rodlie <olear@fxarena.net>\n\nPowered by "+magickV);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadPSDPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamICCIn);
        param->setLabel(kParamICCInLabel);
        param->setHint(kParamICCInHint);
        param->appendOption("None");
        std::vector<std::string> profilesIn;
        _getProFiles(profilesIn, true, "",4);
        for (unsigned int i = 0;i < profilesIn.size();i++)
            param->appendOption(profilesIn[i]);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamICCOut);
        param->setLabel(kParamICCOutLabel);
        param->setHint(kParamICCOutHint);
        param->appendOption("None");
        std::vector<std::string> profilesOut;
        _getProFiles(profilesOut, true, "",1);
        for (unsigned int i = 0;i < profilesOut.size();i++)
            param->appendOption(profilesOut[i]);
        page->addChild(*param);
    }
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
