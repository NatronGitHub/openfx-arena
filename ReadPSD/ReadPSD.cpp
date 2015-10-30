/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "ReadPSD.h"
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#include "ofxsMultiThread.h"
#include <lcms2.h>
#include <dirent.h>
#include <ofxNatron.h>
#include <cstring>

#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ReadPSDOFX"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadPSD"
#define kPluginVersionMajor 2
#define kPluginVersionMinor 3

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
#define kParamICCInSelected "iccInSelected"

#define kParamICCOut "iccOut"
#define kParamICCOutLabel "Output color profile"
#define kParamICCOutHint "ICC RGB output profile\n\nIf image is CMYK/GRAY a colorspace convert will happen."
#define kParamICCOutDefault "sRGB"
#define kParamICCOutSelected "iccOutSelected"

#define kParamICCRGB "iccRGB"
#define kParamICCRGBLabel "Default RGB profile"
#define kParamICCRGBHint "Default RGB profile\n\nUsed when a RGB image is missing an embedded color profile."
#define kParamICCRGBDefault "sRGB"
#define kParamICCRGBSelected "iccRGBSelected"

#define kParamICCCMYK "iccCMYK"
#define kParamICCCMYKLabel "Default CMYK profile"
#define kParamICCCMYKHint "Default CMYK profile\n\nUsed when a CMYK image is missing an embedded color profile."
#define kParamICCCMYKDefault "U.S. Web Coated"
#define kParamICCCMYKSelected "iccCMYKSelected"

#define kParamICCGRAY "iccGRAY"
#define kParamICCGRAYLabel "Default GRAY profile"
#define kParamICCGRAYHint "Default GRAY profile\n\nUsed when a GRAY image is missing an embedded color profile."
#define kParamICCGRAYDefault "Gray linear"
#define kParamICCGRAYSelected "iccGRAYSelected"

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

void _setupChoice(OFX::ChoiceParam *visible, OFX::StringParam *hidden) {
    std::string cString, cCombo;
    hidden->getValue(cString);
    int cID;
    int cCount = visible->getNOptions();
    visible->getValue(cID);
    visible->getOption(cID,cCombo);
    if (!cString.empty()) {
        if (std::strcmp(cCombo.c_str(),cString.c_str())!=0) {
            for(int x = 0; x < cCount; x++) {
                std::string cFound;
                visible->getOption(x,cFound);
                if (!cFound.empty()) {
                    if (std::strcmp(cFound.c_str(),cString.c_str())==0) {
                        visible->setValue(x);
                        break;
                    }
                }
            }
        }
    }
    else {
        if (!cCombo.empty())
            hidden->setValue(cCombo);
    }
}

void _getProFiles(std::vector<std::string> &files, bool desc, std::string filter, int colorspace) {
    std::vector<std::string> paths;
    paths.push_back("/usr/share/color/icc/");
    paths.push_back("\\Windows\\system32\\spool\\drivers\\color\\");
    paths.push_back("/Library/ColorSync/Profiles/");

    // get subfolders
    for (unsigned int i = 0; i < paths.size(); i++) {
        DIR *dp;
        struct dirent *dirp;
        if ((dp=opendir(paths[i].c_str())) != NULL) {
            while ((dirp=readdir(dp)) != NULL) {
                std::ostringstream path;
                std::string proFile = dirp->d_name;
                path << paths[i] << proFile;
                struct stat sb;
                if (stat(path.str().c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
                    if (proFile !="." && proFile != "..")
                        paths.push_back(path.str()+"/");
                }
            }
        }
        if (dp)
            closedir(dp);
    }

    // get profiles from paths
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
    virtual void decode(const std::string& filename, OfxTime time, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds,
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
        decodePlane(filename, time, isPlayback, renderWindow, pixelData, bounds, pixelComponents, pixelComponentCount, rawComps, rowBytes);
    }
    virtual void getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents) OVERRIDE FINAL;
    virtual void decodePlane(const std::string& filename, OfxTime time, bool isPlayback, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, const std::string& rawComponents, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error) OVERRIDE FINAL;
    virtual void restoreState(const std::string& filename) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    void genLayerMenu();
    std::string _filename;
    bool _hasLCMS;
    std::vector<Magick::Image> _psd;
    OFX::ChoiceParam *_iccIn;
    OFX::StringParam *_iccInSelected;
    OFX::ChoiceParam *_iccOut;
    OFX::StringParam *_iccOutSelected;
    OFX::BooleanParam *_doICC;
    OFX::ChoiceParam *_iccRGB;
    OFX::StringParam *_iccRGBSelected;
    OFX::ChoiceParam *_iccCMYK;
    OFX::StringParam *_iccCMYKSelected;
    OFX::ChoiceParam *_iccGRAY;
    OFX::StringParam *_iccGRAYSelected;
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

    _iccInSelected = fetchStringParam(kParamICCInSelected);
    _iccOutSelected = fetchStringParam(kParamICCOutSelected);
    _iccRGBSelected = fetchStringParam(kParamICCRGBSelected);
    _iccCMYKSelected = fetchStringParam(kParamICCCMYKSelected);
    _iccGRAYSelected = fetchStringParam(kParamICCGRAYSelected);

    assert(_outputComponents && _iccIn && _iccOut && _doICC && _iccRGB && _iccCMYK && _iccGRAY && _iccRender && _iccBlack && _imageLayer && _offsetLayer && _iccInSelected && _iccOutSelected && _iccRGBSelected && _iccCMYKSelected && _iccGRAYSelected);

    _setupChoice(_iccIn, _iccInSelected);
    _setupChoice(_iccOut, _iccOutSelected);
    _setupChoice(_iccRGB, _iccRGBSelected);
    _setupChoice(_iccCMYK, _iccCMYKSelected);
    _setupChoice(_iccGRAY, _iccGRAYSelected);
}

ReadPSDPlugin::~ReadPSDPlugin()
{
}

void ReadPSDPlugin::genLayerMenu()
{
    if (gHostIsNatron) {
        _imageLayer->resetOptions();
        int startLayer = 0;
        if (!_psd.empty() && _psd[0].format() == "Adobe Photoshop bitmap") {
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
        if (!_psd.empty() && _psd[0].format() == "Adobe Photoshop bitmap")
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

void ReadPSDPlugin::decodePlane(const std::string& filename, OfxTime time, bool /*isPlayback*/, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds,
                                 OFX::PixelComponentEnum /*pixelComponents*/, int /*pixelComponentCount*/, const std::string& rawComponents, int /*rowBytes*/)
{
    #ifdef DEBUG
    std::cout << "decodePlane ..." << std::endl;
    #endif

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    int offsetX = 0;
    int offsetY = 0;
    int layer = 0;
    int width = bounds.x2;
    int height = bounds.y2;
    std::string layerName;
    bool color = false;
    int iccRender = 0;
    bool iccBlack = false;
    int imageLayer = 0;
    bool offsetLayer = false;
    std::vector<std::string> layerChannels = OFX::mapPixelComponentCustomToLayerChannels(rawComponents);
    int numChannels = layerChannels.size();
    std::string iccProfileIn, iccProfileOut, iccProfileRGB, iccProfileCMYK, iccProfileGRAY;
    _iccInSelected->getValueAtTime(time, iccProfileIn);
    _iccOutSelected->getValueAtTime(time, iccProfileOut);
    _iccRGBSelected->getValueAtTime(time, iccProfileRGB);
    _iccCMYKSelected->getValueAtTime(time, iccProfileCMYK);
    _iccGRAYSelected->getValueAtTime(time, iccProfileGRAY);
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
                    offsetX = _psd[i].page().xOff();
                    offsetY = _psd[i].page().yOff();
                }
                layer = i;
                break;
            }
        }
    }
    else { // no multiplane
        if (imageLayer>0 || _psd[imageLayer].format()!="Adobe Photoshop bitmap") {
            if (offsetLayer) {
                offsetX = _psd[imageLayer].page().xOff();
                offsetY = _psd[imageLayer].page().yOff();
            }
        }
        layer = imageLayer;
    }

    // Get image
    Magick::Image image;
    if (_filename!=filename) { // anim?
        std::ostringstream newFile;
        newFile << filename << "[" << layer << "]";
        image.read(newFile.str().c_str());
    }
    else
        image = _psd[layer];

    // color management
    if (color && _hasLCMS) {
        // cascade menu
        if (gHostIsNatron) {
            iccProfileIn.erase(0,2);
            iccProfileOut.erase(0,2);
            iccProfileRGB.erase(0,2);
            iccProfileCMYK.erase(0,2);
            iccProfileGRAY.erase(0,2);
        }
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
        setPersistentMessage(OFX::Message::eMessageError, "", "LCMS support missing in ImageMagick, unable to use color management");
    }

    // Return image
    Magick::Image container(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    container.composite(image,offsetX,offsetY,Magick::OverCompositeOp);
    container.flip();
    //width = bounds.x2;
    //height = bounds.y2;
    int widthstep = width*4;
    int imageSize = width*height*4;
    float* imageBlock;
    imageBlock = new float[imageSize];
    container.write(0,0,width,height,"RGBA",Magick::FloatPixel,imageBlock);
    for(int y = renderWindow.y1; y < (renderWindow.y1 + height); y++) {
        float *dstPix = (float*)(pixelData + y * widthstep + renderWindow.x1);
        float *srcPix = (float*)(imageBlock + (y * widthstep + renderWindow.x1));
        for(int x = renderWindow.x1; x < (renderWindow.x1 + width); x++) {
            dstPix[0] = srcPix[0]*srcPix[3];
            dstPix[1] = srcPix[1]*srcPix[3];
            dstPix[2] = srcPix[2]*srcPix[3];
            dstPix[3] = srcPix[3];
            dstPix+=4;
            srcPix+=4;
        }
    }
    free(imageBlock);
}

void ReadPSDPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName)
{
    if (paramName == kParamICCIn) {
        std::string profile;
        int proID;
        _iccIn->getValueAtTime(args.time, proID);
        _iccIn->getOption(proID,profile);
        _iccInSelected->setValueAtTime(args.time, profile);
    }
    else if (paramName == kParamICCOut) {
        std::string profile;
        int proID;
        _iccOut->getValueAtTime(args.time, proID);
        _iccOut->getOption(proID,profile);
        _iccOutSelected->setValueAtTime(args.time, profile);
    }
    else if (paramName == kParamICCRGB) {
        std::string profile;
        int proID;
        _iccRGB->getValueAtTime(args.time, proID);
        _iccRGB->getOption(proID,profile);
        _iccRGBSelected->setValueAtTime(args.time, profile);
    }
    else if (paramName == kParamICCCMYK) {
        std::string profile;
        int proID;
        _iccCMYK->getValueAtTime(args.time, proID);
        _iccCMYK->getOption(proID,profile);
        _iccCMYKSelected->setValueAtTime(args.time, profile);
    }
    else if (paramName == kParamICCGRAY) {
        std::string profile;
        int proID;
        _iccGRAY->getValueAtTime(args.time, proID);
        _iccGRAY->getOption(proID,profile);
        _iccGRAYSelected->setValueAtTime(args.time, profile);
    }
    else {
        GenericReaderPlugin::changedParam(args,paramName);
    }
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
    #ifdef DEBUG
    std::cout << "restoreState ..." << std::endl;
    #endif

    // Set max threads allowed by host
    unsigned int threads = 0;
    threads = OFX::MultiThread::getNumCPUs();
    if (threads>0) {
        Magick::ResourceLimits::thread(threads);
        #ifdef DEBUG
        std::cout << "Setting max threads to " << threads << std::endl;
        #endif
    }

    _psd.clear();
    _maxWidth = 0;
    _maxHeight = 0;
    int layer = 0;
    _imageLayer->getValue(layer);
    try {
        if (!filename.empty()) {
            Magick::readImages(&_psd, filename);
        }
    }
    catch(Magick::Exception) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Unable to read image");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    genLayerMenu();

    #ifdef DEBUG_MAGICK
    _psd[layer].debug(true);
    #endif

    if (!_psd.empty() && _psd[layer].columns()>0 && _psd[layer].rows()>0) {
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
    _ocio->setInputColorspace("sRGB");
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

    size_t magickNumber;
    std::string magickString = MagickCore::GetMagickVersion(&magickNumber);
    std::string plugCopyright = "\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.\n\nLittle CMS is Copyright 2010-2015 Marti Maria Saguer. All rights reserved.\n\nLittle CMS is distributed under the MIT license agreement.";
    # ifdef OFX_IO_USING_OCIO
    plugCopyright.append("\n\nOpenColorIO is Copyright 2003-2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved.\n\nOpenColorIO is distributed under a BSD license.");
    # endif // OFX_IO_USING_OCIO
    desc.setPluginDescription("Read Photoshop/GIMP/Cinepaint (RGB/CMYK/GRAY) image formats with ICC color management.\n\nPowered by Little CMS v2 http://www.littlecms.com/ and "+magickString+plugCopyright);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ReadPSDPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

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
        if (gHostIsNatron)
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);
        param->setLabel(kParamICCRGBLabel);
        param->setHint(kParamICCRGBHint);
        param->appendOption("None");
        std::vector<std::string> profilesRGB;
        _getProFiles(profilesRGB, true, "",1); // get RGB profiles
        int defaultOpt = 0;
        for (unsigned int i = 0;i < profilesRGB.size();i++) {
            std::string proItem;
            std::string proName = profilesRGB[i];
            if (gHostIsNatron) {
                proItem=proName[0];
                proItem.append("/"+proName);
            }
            else
                proItem=proName;
            param->appendOption(proItem);
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
        if (gHostIsNatron)
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);
        param->setLabel(kParamICCCMYKLabel);
        param->setHint(kParamICCCMYKHint);
        param->appendOption("None");
        std::vector<std::string> profilesCMYK;
        _getProFiles(profilesCMYK, true, "",2); // get CMYK profiles
        int defaultOpt = 0;
        for (unsigned int i = 0;i < profilesCMYK.size();i++) {
            std::string proItem;
            std::string proName = profilesCMYK[i];
            if (gHostIsNatron) {
                proItem=proName[0];
                proItem.append("/"+proName);
            }
            else
                proItem=proName;
            param->appendOption(proItem);
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
        if (gHostIsNatron)
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);
        param->setLabel(kParamICCGRAYLabel);
        param->setHint(kParamICCGRAYHint);
        param->appendOption("None");
        std::vector<std::string> profilesGRAY;
        _getProFiles(profilesGRAY, true, "",3); // get GRAY profiles
        int defaultOpt = 0;
        for (unsigned int i = 0;i < profilesGRAY.size();i++) {
            std::string proItem;
            std::string proName = profilesGRAY[i];
            if (gHostIsNatron) {
                proItem=proName[0];
                proItem.append("/"+proName);
            }
            else
                proItem=proName;
            param->appendOption(proItem);
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
        if (gHostIsNatron)
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);
        param->setLabel(kParamICCInLabel);
        param->setHint(kParamICCInHint);
        param->appendOption("None");
        std::vector<std::string> profilesIn;
        _getProFiles(profilesIn, true, "",4); // get RGB/CMYK/GRAY profiles
        for (unsigned int i = 0;i < profilesIn.size();i++) {
            std::string proItem;
            std::string proName = profilesIn[i];
            if (gHostIsNatron) {
                proItem=proName[0];
                proItem.append("/"+proName);
            }
            else
                proItem=proName;
            param->appendOption(proItem);
        }
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor* param = desc.defineChoiceParam(kParamICCOut);
        if (gHostIsNatron)
            param->setCascading(OFX::getImageEffectHostDescription()->supportsCascadingChoices);
        param->setLabel(kParamICCOutLabel);
        param->setHint(kParamICCOutHint);
        param->appendOption("None");
        std::vector<std::string> profilesOut;
        _getProFiles(profilesOut, true, "",1); // get RGB profiles
        int defaultOpt = 0;
        for (unsigned int i = 0;i < profilesOut.size();i++) {
            std::string proItem;
            std::string proName = profilesOut[i];
            if (gHostIsNatron) {
                proItem=proName[0];
                proItem.append("/"+proName);
            }
            else
                proItem=proName;
            param->appendOption(proItem);
            if (profilesOut[i].find(kParamICCOutDefault) != std::string::npos) // set default
                defaultOpt=i;
        }
        if (defaultOpt>0) {
            defaultOpt++;
            param->setDefault(defaultOpt);
        }
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamICCInSelected);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);

        #ifdef DEBUG
        param->setIsSecret(false);
        #else
        param->setIsSecret(true);
        #endif

        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamICCOutSelected);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);

        #ifdef DEBUG
        param->setIsSecret(false);
        #else
        param->setIsSecret(true);
        #endif

        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamICCRGBSelected);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);

        #ifdef DEBUG
        param->setIsSecret(false);
        #else
        param->setIsSecret(true);
        #endif

        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamICCCMYKSelected);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);

        #ifdef DEBUG
        param->setIsSecret(false);
        #else
        param->setIsSecret(true);
        #endif

        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamICCGRAYSelected);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);

        #ifdef DEBUG
        param->setIsSecret(false);
        #else
        param->setIsSecret(true);
        #endif

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
