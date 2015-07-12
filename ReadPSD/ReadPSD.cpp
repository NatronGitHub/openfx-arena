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
#include <ofxNatron.h>

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadPSDOFX"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadPSD"
#define kPluginVersionMajor 2
#define kPluginVersionMinor 0

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar true

#define kParamICC "icc"
#define kParamICCLabel "Color management"
#define kParamICCHint "Enable/Disable ICC color management\n\nRequires installed ICC v2/v4 color profiles."
#define kParamICCDefault false

#define kParamICCIn "iccIn"
#define kParamICCInLabel "Input color profile"
#define kParamICCInHint "ICC input profile\n\nIf profile colorspace differs from image colorspace then a colorspace convert will happen."

#define kParamICCOut "iccOut"
#define kParamICCOutLabel "Output color profile"
#define kParamICCOutHint "ICC RGB output profile\n\nIf image is CMYK/GRAY a colorspace convert will happen."
#define kParamICCOutDefault "sRGB"

#define kParamICCRGB "iccRGB"
#define kParamICCRGBLabel "Default RGB profile"
#define kParamICCRGBHint "Default RGB profile\n\nUsed when a RGB image is missing an embedded color profile."
#define kParamICCRGBDefault "sRGB"

#define kParamICCCMYK "iccCMYK"
#define kParamICCCMYKLabel "Default CMYK profile"
#define kParamICCCMYKHint "Default CMYK profile\n\nUsed when a CMYK image is missing an embedded color profile."
#define kParamICCCMYKDefault "U.S. Web Coated"

#define kParamICCGRAY "iccGRAY"
#define kParamICCGRAYLabel "Default GRAY profile"
#define kParamICCGRAYHint "Default GRAY profile\n\nUsed when a GRAY image is missing an embedded color profile."
#define kParamICCGRAYDefault "Gray linear"

#define kParamICCRender "renderingIntent"
#define kParamICCRenderLabel "Rendering intent"
#define kParamICCRenderHint "Rendering intent specifies the style of reproduction to be used."
#define kParamICCRenderDefault 2 //Perceptual

#define kParamICCBlack "blackPoint"
#define kParamICCBlackLabel "Black point"
#define kParamICCBlackHint "Enable/Disable black point compensation"
#define kParamICCBlackDefault false

#define kParamImageLayer "layer"
#define kParamImageLayerLabel "Image layer"
#define kParamImageLayerHint "Select image layer\n\nThe recommended way to access layers is through a merge/shuffle node (multi-plane)."
#define kParamImageLayerDefault 0

#define kParamOffsetLayer "offset"
#define kParamOffsetLayerLabel "Offset layers"
#define kParamOffsetLayerHint "Enable/Disable layer offset"
#define kParamOffsetLayerDefault true

static bool gHostIsNatron   = false;

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
        if (dp)
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
    void genLayerMenu();
    std::string _filename;
    bool _hasLCMS;
    std::vector<Magick::Image> _psd;
    OFX::ChoiceParam *_iccIn;
    OFX::ChoiceParam *_iccOut;
    OFX::BooleanParam *_doICC;
    OFX::ChoiceParam *_iccRGB;
    OFX::ChoiceParam *_iccCMYK;
    OFX::ChoiceParam *_iccGRAY;
    OFX::ChoiceParam *_iccRender;
    OFX::BooleanParam *_iccBlack;
    OFX::ChoiceParam *_imageLayer;
    OFX::BooleanParam *_offsetLayer;
    int _maxWidth;
    int _maxHeight;
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
,_maxWidth(0)
,_maxHeight(0)
{
    Magick::InitializeMagick(NULL);

    std::string delegates = MagickCore::GetMagickDelegates();
    if (delegates.find("lcms") != std::string::npos)
        _hasLCMS = true;

    _iccIn = fetchChoiceParam(kParamICCIn);
    _iccOut = fetchChoiceParam(kParamICCOut);
    _iccRGB = fetchChoiceParam(kParamICCRGB);
    _iccCMYK = fetchChoiceParam(kParamICCCMYK);
    _iccGRAY = fetchChoiceParam(kParamICCGRAY);
    _doICC = fetchBooleanParam(kParamICC);
    _iccRender = fetchChoiceParam(kParamICCRender);
    _iccBlack = fetchBooleanParam(kParamICCBlack);
    _imageLayer = fetchChoiceParam(kParamImageLayer);
    _offsetLayer = fetchBooleanParam(kParamOffsetLayer);

    assert(_outputComponents && _iccIn && _iccOut && _doICC && _iccRGB && _iccCMYK && _iccGRAY && _iccRender && _iccBlack && _imageLayer && _offsetLayer);
}

ReadPSDPlugin::~ReadPSDPlugin()
{
}

void ReadPSDPlugin::genLayerMenu()
{
    if (gHostIsNatron) {
        _imageLayer->resetOptions();
        int startLayer = 0;
        if (_psd[0].format()=="Adobe Photoshop bitmap") {
            _imageLayer->appendOption("Default");
            startLayer++; // first layer in a PSD is a comp
        }
        for (int i = startLayer; i < (int)_psd.size(); i++) {
            std::ostringstream layerName;
            layerName << _psd[i].label();
            if (layerName.str().empty())
                layerName << "Layer " << i; // add a label if empty
            _imageLayer->appendOption(layerName.str());
        }
    }
}

void ReadPSDPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    #ifdef DEBUG
    std::cout << "getClipComponents ..." << std::endl;
    #endif
    assert(isMultiPlanar());
    clipComponents.addClipComponents(*_outputClip, getOutputComponents());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (_psd.size()>0 && gHostIsNatron) { // what about nuke?
        int startLayer = 0;
        if (_psd[0].format()=="Adobe Photoshop bitmap")
            startLayer++; // first layer in a PSD is a comp
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
    int iccProfileRGBID = 0;
    std::string iccProfileRGB;
    int iccProfileCMYKID = 0;
    std::string iccProfileCMYK;
    int iccProfileGRAYID = 0;
    std::string iccProfileGRAY;
    bool color = false;
    int iccRender = 0;
    bool iccBlack = false;
    int imageLayer = 0;
    bool offsetLayer = false;
    std::vector<std::string> layerChannels = OFX::mapPixelComponentCustomToLayerChannels(rawComponents);
    int numChannels = layerChannels.size();
    _iccIn->getValueAtTime(time, iccProfileInID);
    _iccIn->getOption(iccProfileInID, iccProfileIn);
    _iccOut->getValueAtTime(time, iccProfileOutID);
    _iccOut->getOption(iccProfileOutID, iccProfileOut);
    _iccRGB->getValueAtTime(time, iccProfileRGBID);
    _iccRGB->getOption(iccProfileRGBID, iccProfileRGB);
    _iccCMYK->getValueAtTime(time, iccProfileCMYKID);
    _iccCMYK->getOption(iccProfileCMYKID, iccProfileCMYK);
    _iccGRAY->getValueAtTime(time, iccProfileGRAYID);
    _iccGRAY->getOption(iccProfileGRAYID, iccProfileGRAY);
    _doICC->getValueAtTime(time, color);
    _iccRender->getValueAtTime(time, iccRender);
    _iccBlack->getValueAtTime(time, iccBlack);
    _imageLayer->getValueAtTime(time, imageLayer);
    _offsetLayer->getValueAtTime(time, offsetLayer);

    // Get multiplane layer
    if (numChannels==5) // layer+R+G+B+A
        layerName=layerChannels[0];
    if (!layerName.empty()) {
        for (size_t i = 0; i < _psd.size(); i++) {
            bool foundLayer = false;
            std::ostringstream psdLayer;
            psdLayer << "Image Layer #" << i; // if layer name is empty
            if (_psd[i].label()==layerName)
                foundLayer = true;
            if (psdLayer.str()==layerName && !foundLayer)
                foundLayer = true;
            if (foundLayer) {
                if (offsetLayer) {
                    //if ((int)_psd[i].columns()!=bounds.x2)
                        offsetX = _psd[i].page().xOff();
                    //if ((int)_psd[i].rows()!=bounds.y2)
                        offsetY = _psd[i].page().yOff();
                }
                layer = i;
                break;
            }
        }
    }
    else { // no multiplane
        /*if (imageLayer==0 && _psd[imageLayer].format()=="Adobe Photoshop bitmap") {
            if ((int)_psd[imageLayer].columns()<width)
                offsetX = width-(int)_psd[imageLayer].columns();
            if ((int)_psd[imageLayer].rows()<height)
                offsetY = height-(int)_psd[imageLayer].rows();
        }*/
        if (imageLayer>0 || _psd[imageLayer].format()!="Adobe Photoshop bitmap") {
            if (offsetLayer) {
                //if ((int)_psd[imageLayer].columns()!=bounds.x2)
                    offsetX = _psd[imageLayer].page().xOff();
                //if ((int)_psd[imageLayer].rows()!=bounds.y2)
                    offsetY = _psd[imageLayer].page().yOff();
            }
        }
        layer = imageLayer;
    }

    // Get image
    Magick::Image image;
    image = _psd[layer];

    // color management
    if (color && _hasLCMS) {
        // blackpoint
        if (iccBlack)
            image.blackPointCompensation(true);
        // render intent
        switch (iccRender) {
        case 1: // SaturationIntent
            image.renderingIntent(Magick::SaturationIntent);
            break;
        case 2: // PerceptualIntent
            image.renderingIntent(Magick::PerceptualIntent);
            break;
        case 3: // AbsoluteIntent
            image.renderingIntent(Magick::AbsoluteIntent);
            break;
        case 4: // RelativeIntent
            image.renderingIntent(Magick::RelativeIntent);
            break;
        default:
            //
            break;
        }
        // default profile
        std::string defaultProfile;
        if (image.iccColorProfile().length()==0) {
            std::vector<std::string> profileDef;
            switch(image.colorSpace()) {
            case Magick::RGBColorspace:
                if (!iccProfileRGB.empty())
                    _getProFiles(profileDef, false, iccProfileRGB,1);
                break;
            case Magick::sRGBColorspace:
                if (!iccProfileRGB.empty())
                    _getProFiles(profileDef, false, iccProfileRGB,1);
                break;
            case Magick::scRGBColorspace:
                if (!iccProfileRGB.empty())
                    _getProFiles(profileDef, false, iccProfileRGB,1);
                break;
            case Magick::CMYKColorspace:
                if (!iccProfileCMYK.empty())
                    _getProFiles(profileDef, false, iccProfileCMYK,2);
                break;
            case Magick::GRAYColorspace:
                if (!iccProfileGRAY.empty())
                    _getProFiles(profileDef, false, iccProfileGRAY,3);
                break;
            default:
                //
                break;
            }
            if (profileDef.size()==1)
                defaultProfile=profileDef[0];
        }
        // ICC input
        if (!iccProfileIn.empty() && iccProfileIn.find("None") == std::string::npos) {
            std::vector<std::string> profile;
            _getProFiles(profile, false, iccProfileIn,4);
            if (profile.size()==1) {
                if (!profile[0].empty()) {
                    if (!defaultProfile.empty()) { // apply default profile if not exist
                        Magick::Blob iccBlobDef;
                        Magick::Image iccExtractDef(defaultProfile);
                        iccExtractDef.write(&iccBlobDef);
                        if (iccBlobDef.length()>0)
                            image.profile("ICC",iccBlobDef);
                    }
                    Magick::Blob iccBlob;
                    Magick::Image iccExtract(profile[0]);
                    iccExtract.write(&iccBlob);
                    try { // catch final convert errors, like wrong profile compared to colorspace etc
                        image.profile("ICC",iccBlob);
                    }
                    catch(Magick::Exception &error) {
                        setPersistentMessage(OFX::Message::eMessageError, "", error.what());
                        OFX::throwSuiteStatusException(kOfxStatFailed);
                    }
                }
            }
        }
        // ICC output
        if (!iccProfileOut.empty() && iccProfileOut.find("None") == std::string::npos) {
            std::vector<std::string> profile;
            _getProFiles(profile, false, iccProfileOut,1);
            if (profile.size()==1) {
                if (!profile[0].empty()) {
                    if (!defaultProfile.empty() && (iccProfileIn.find("None") != std::string::npos)) { // apply default profile if not exist
                        Magick::Blob iccBlobDef;
                        Magick::Image iccExtractDef(defaultProfile);
                        iccExtractDef.write(&iccBlobDef);
                        if (iccBlobDef.length()>0)
                            image.profile("ICC",iccBlobDef);
                    }
                    Magick::Blob iccBlob;
                    Magick::Image iccExtract(profile[0]);
                    iccExtract.write(&iccBlob);
                    if (iccBlob.length()>0) {
                        try { // catch final convert errors, like wrong profile compared to colorspace etc
                            image.profile("ICC",iccBlob);
                        }
                        catch(Magick::Exception &error) {
                            setPersistentMessage(OFX::Message::eMessageError, "", error.what());
                            OFX::throwSuiteStatusException(kOfxStatFailed);
                        }
                    }
                }
            }
        }
    }
    else if (color && !_hasLCMS) {
        setPersistentMessage(OFX::Message::eMessageError, "", "LCMS support missing, unable to use color management");
    }

    // Return image (comping on a empty canvas makes things easier, modify when #126 is done)
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
    if (_maxWidth>0 && _maxHeight>0) {
        bounds->x1 = 0;
        bounds->x2 = _maxWidth;
        bounds->y1 = 0;
        bounds->y2 = _maxHeight;
        *par = 1.0;
    }
    return true;
}

void ReadPSDPlugin::restoreState(const std::string& filename)
{
    _psd.clear();
    _maxWidth = 0;
    _maxHeight = 0;
    int layer = 0;
    _imageLayer->getValue(layer);
    try {
        Magick::readImages(&_psd, filename);
    }
    catch(Magick::Exception) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    genLayerMenu();
    #ifdef DEBUG
    _psd[layer].debug(true);
    #endif
    if (_psd[layer].columns()>0 && _psd[layer].rows()>0) {
        _filename = filename;
        for (int i = 0; i < (int)_psd.size(); i++) {
            if ((int)_psd[i].columns()>_maxWidth)
                _maxWidth = (int)_psd[i].columns();
            if ((int)_psd[i].rows()>_maxHeight)
                _maxHeight = (int)_psd[i].rows();
        }
    }
    else {
        _psd.clear();
        _maxWidth = 0;
        _maxHeight = 0;
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
    default:
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
    gHostIsNatron = (OFX::getImageEffectHostDescription()->hostName == kNatronOfxHostName);

    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamImageLayer);
        param->setLabel(kParamImageLayerLabel);
        param->setHint(kParamImageLayerHint);
        param->appendOption("Default");
        param->appendOption("Layer 1"); // for non-natron:
        param->appendOption("Layer 2");
        param->appendOption("Layer 3");
        param->appendOption("Layer 4");
        param->appendOption("Layer 5");
        param->appendOption("Layer 6");
        param->appendOption("Layer 7");
        param->appendOption("Layer 8");
        param->appendOption("Layer 9");
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamOffsetLayer);
        param->setLabel(kParamOffsetLayerLabel);
        param->setHint(kParamOffsetLayerHint);
        param->setDefault(kParamOffsetLayerDefault);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamICC);
        param->setLabel(kParamICCLabel);
        param->setHint(kParamICCHint);
        param->setDefault(kParamICCDefault);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamICCRGB);
        param->setLabel(kParamICCRGBLabel);
        param->setHint(kParamICCRGBHint);
        param->appendOption("None");
        std::vector<std::string> profilesRGB;
        _getProFiles(profilesRGB, true, "",1); // get RGB profiles
        int defaultOpt = 0;
        for (unsigned int i = 0;i < profilesRGB.size();i++) {
            param->appendOption(profilesRGB[i]);
            if (profilesRGB[i].find(kParamICCRGBDefault) != std::string::npos) // set default
                defaultOpt=i;
        }
        if (defaultOpt>0) {
            defaultOpt++;
            param->setDefault(defaultOpt);
        }
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamICCCMYK);
        param->setLabel(kParamICCCMYKLabel);
        param->setHint(kParamICCCMYKHint);
        param->appendOption("None");
        std::vector<std::string> profilesCMYK;
        _getProFiles(profilesCMYK, true, "",2); // get CMYK profiles
        int defaultOpt = 0;
        for (unsigned int i = 0;i < profilesCMYK.size();i++) {
            param->appendOption(profilesCMYK[i]);
            if (profilesCMYK[i].find(kParamICCCMYKDefault) != std::string::npos) // set default
                defaultOpt=i;
        }
        if (defaultOpt>0) {
            defaultOpt++;
            param->setDefault(defaultOpt);
        }
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamICCGRAY);
        param->setLabel(kParamICCGRAYLabel);
        param->setHint(kParamICCGRAYHint);
        param->appendOption("None");
        std::vector<std::string> profilesGRAY;
        _getProFiles(profilesGRAY, true, "",3); // get GRAY profiles
        int defaultOpt = 0;
        for (unsigned int i = 0;i < profilesGRAY.size();i++) {
            param->appendOption(profilesGRAY[i]);
            if (profilesGRAY[i].find(kParamICCGRAYDefault) != std::string::npos) // set default
                defaultOpt=i;
        }
        if (defaultOpt>0) {
            defaultOpt++;
            param->setDefault(defaultOpt);
        }
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamICCRender);
        param->setLabel(kParamICCRenderLabel);
        param->setHint(kParamICCRenderHint);
        param->appendOption("Undefined");
        param->appendOption("Saturation");
        param->appendOption("Perceptual");
        param->appendOption("Absolute");
        param->appendOption("Relative");
        param->setDefault(kParamICCRenderDefault);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamICCBlack);
        param->setLabel(kParamICCBlackLabel);
        param->setHint(kParamICCBlackHint);
        param->setDefault(kParamICCBlackDefault);
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamICCIn);
        param->setLabel(kParamICCInLabel);
        param->setHint(kParamICCInHint);
        param->appendOption("None");
        std::vector<std::string> profilesIn;
        _getProFiles(profilesIn, true, "",4); // get RGB/CMYK/GRAY profiles
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
        _getProFiles(profilesOut, true, "",1); // get RGB profiles
        int defaultOpt = 0;
        for (unsigned int i = 0;i < profilesOut.size();i++) {
            param->appendOption(profilesOut[i]);
            if (profilesOut[i].find(kParamICCOutDefault) != std::string::npos) // set default
                defaultOpt=i;
        }
        if (defaultOpt>0) {
            defaultOpt++;
            param->setDefault(defaultOpt);
        }
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
