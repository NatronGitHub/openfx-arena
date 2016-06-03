/*
# Copyright (c) 2015, Ole-André Rodlie <olear@dracolinux.org>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
*/

#ifndef _WINDOWS // "povray on windows" is not the same as "povray on unix". We need a "povray for unix" build on mingw for this plugin to work on windows.

#include "ofxsMacros.h"
#include "ofxsImageEffect.h"
#include <Magick++.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <fstream>

#define kPluginName "PovRayOFX"
#define kPluginGrouping "Extra/Draw"
#define kPluginIdentifier "net.fxarena.openfx.PovRay"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 2

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderUnsafe // seems to work best(?)
#define kHostFrameThreading false

#define kParamScene "scene"
#define kParamSceneLabel "Scene"
#define kParamSceneHint "Add or write your POV-Ray scene here. Examples can be found at https://github.com/POV-Ray/povray/tree/master/distribution/scenes"
#define kParamSceneDefault "// This work is licensed under the Creative Commons Attribution 3.0 Unported License.\n// To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/\n// or send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View,\n// California, 94041, USA.\n\n// Persistence Of Vision raytracer sample file.\n// File by Dan Farmer\n// Demonstrates glass textures, CGS with box primitives, one of Mike Miller's\n// fabulous marble textures, modified with an octaves change, and doesn't\n// make a half-bad image, either.  Interesting lighting effect, too.\n#version 3.7;\n\nglobal_settings {\nassumed_gamma 2.0\nmax_trace_level 5\n}\n\n#include \"colors.inc\"\n#include \"shapes.inc\"\n#include \"textures.inc\"\n#include \"glass.inc\"\n#include \"stones.inc\"\n\ncamera {\nlocation  <0.75, 3.5, -3.5>\nangle 100 // direction <0.0,  0.0,  0.5>       //  wide-angle view\nright   x*image_width/image_height\nlook_at   <0,    0,   -1>\n}\n\n// Light sources, two to the front, right, on from the left, rear.\nlight_source {<-30, 11,  20> color White}\nlight_source {< 31, 12, -20> color White}\nlight_source {< 32, 11, -20> color LightGray}\n\nunion {\n// A green glass ball inside of a box-shaped frame\nsphere { <0, 0, 0>, 1.75\n// converted to material 07Aug2008 (jh)\nmaterial {\ntexture {\npigment {color green 0.90 filter 0.85}\nfinish {\nphong 1 phong_size 300         // Very tight highlights\nreflection 0.15                // Needs a little reflection added\n}\n}\ninterior{\ncaustics 1.0\nior 1.5\n}\n}\n}\n\n// A box-shaped frame surrounding a green glass ball\ndifference {\nobject {UnitBox scale 1.5}     // The outside dimensions\n\n// And some square holes in all sides.  Note that each of\n// these boxes that are going to be subtracted has one vector\n// scaled just slightly larger than the outside box.  The other\n// two vectors determine the size of the hole.\n// Clip some sqr holes in the box to make a 3D box frame\nobject{UnitBox scale <1.51, 1.25, 1.25> }   // clip x\nobject{UnitBox scale <1.25, 1.51, 1.25> }   // clip y\nobject{UnitBox scale <1.25, 1.25, 1.51> }   // clip z\n\npigment { red 0.75 green 0.75 blue 0.85 }\nfinish {\nambient 0.2\ndiffuse 0.7\nreflection 0.15\nbrilliance 8\nspecular 1\nroughness 0.01\n}\n// Same as radius of glass sphere, not the box!\nbounded_by {object {UnitBox scale 1.75}}\n}\nrotate y*45\n}\n\nplane { y, -1.5\ntexture {\nT_Stone1\npigment {\noctaves 3\nrotate 90*z\n}\nfinish { reflection 0.10 }\n}\n}"

#define kParamQuality "quality"
#define kParamQualityLabel "Quality"
#define kParamQualityHint "Set render quality"
#define kParamQualityDefault 9

#define kParamAntialiasing "antialiasing"
#define kParamAntialiasingLabel "Antialiasing"
#define kParamAntialiasingHint "Set render antialiasing"
#define kParamAntialiasingDefault 0

#define kParamAlpha "alpha"
#define kParamAlphaLabel "Alpha"
#define kParamAlphaHint "Use alpha channel for transparency mask"
#define kParamAlphaDefault true

#define kParamWidth "width"
#define kParamWidthLabel "Width"
#define kParamWidthHint "Set canvas width, default (0) is project format"
#define kParamWidthDefault 0

#define kParamHeight "height"
#define kParamHeightLabel "Height"
#define kParamHeightHint "Set canvas height, default (0) is project format"
#define kParamHeightDefault 0

#define kParamPovPath "povPath"
#define kParamPovPathLabel "Executable"
#define kParamPovPathHint "Path to POV-Ray executable"
#define kParamPovPathDefault ""

#define kParamPovInc "povInc"
#define kParamPovIncLabel "Includes"
#define kParamPovIncHint "Path to POV-Ray includes"

#define kParamPovCmd "povCmd"
#define kParamPovCmdLabel "Commandline"
#define kParamPovCmdHint "Add additional POV-Ray commands here, be careful!"

#define kParamSF "startFrame"
#define kParamSFLabel "Start Frame"
#define kParamSFHint "Set start frame"
#define kParamSFDefault 0

#define kParamEF "endFrame"
#define kParamEFLabel "End Frame"
#define kParamEFHint "Set end frame"
#define kParamEFDefault 0

using namespace OFX;

static bool fileExists(const std::string& filename)
{
    std::ifstream f(filename.c_str());
    bool ret = f.good();
    f.close();
    return ret;
}

class PovRayPlugin : public OFX::ImageEffect
{
public:
    PovRayPlugin(OfxImageEffectHandle handle);
    virtual ~PovRayPlugin();

    /* Override the render */
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;

    /* override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;

    // override the rod call
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;

private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *dstClip_;
    OFX::StringParam *scene_;
    OFX::IntParam *width_;
    OFX::IntParam *height_;
    OFX::IntParam *quality_;
    OFX::IntParam *antialiasing_;
    OFX::BooleanParam *alpha_;
    OFX::StringParam *povpath_;
    OFX::StringParam *povinc_;
    OFX::StringParam *povcmd_;
    OFX::IntParam *startFrame_;
    OFX::IntParam *endFrame_;
};

PovRayPlugin::PovRayPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, width_(0)
, height_(0)
{
    Magick::InitializeMagick(NULL);

    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    scene_ = fetchStringParam(kParamScene);
    width_ = fetchIntParam(kParamWidth);
    height_ = fetchIntParam(kParamHeight);
    quality_ = fetchIntParam(kParamQuality);
    antialiasing_ = fetchIntParam(kParamAntialiasing);
    alpha_ = fetchBooleanParam(kParamAlpha);
    povpath_ = fetchStringParam(kParamPovPath);
    povinc_ = fetchStringParam(kParamPovInc);
    povcmd_ = fetchStringParam(kParamPovCmd);
    startFrame_ = fetchIntParam(kParamSF);
    endFrame_ = fetchIntParam(kParamEF);

    assert(scene_ && width_ && height_ && quality_ && antialiasing_ && alpha_ && povpath_ && povinc_ && povcmd_ && startFrame_ && endFrame_);
}

PovRayPlugin::~PovRayPlugin()
{
}

/* Override the render */
void PovRayPlugin::render(const OFX::RenderArguments &args)
{
    // renderscale
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // dstclip
    if (!dstClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);

    // get dstclip
    std::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // renderscale
    if (dstImg->getRenderScale().x != args.renderScale.x ||
        dstImg->getRenderScale().y != args.renderScale.y ||
        dstImg->getField() != args.fieldToRender) {
        setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get bitdepth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get channels
    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != OFX::ePixelComponentRGBA) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds
    OfxRectI dstBounds = dstImg->getBounds();
    OfxRectI dstRod = dstImg->getRegionOfDefinition();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Get params
    std::string scene, povpath, povinc, povcmd;
    int quality, antialiasing, startFrame, endFrame;
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;
    bool alpha = false;
    scene_->getValueAtTime(args.time, scene);
    quality_->getValueAtTime(args.time, quality);
    antialiasing_->getValueAtTime(args.time, antialiasing);
    alpha_->getValueAtTime(args.time, alpha);
    povpath_->getValueAtTime(args.time, povpath);
    povinc_->getValueAtTime(args.time, povinc);
    povcmd_->getValueAtTime(args.time, povcmd);
    startFrame_->getValueAtTime(args.time, startFrame);
    endFrame_->getValueAtTime(args.time, endFrame);

    // Temp scene
    const char *folder = getenv("TMPDIR");
    if (folder==0)
        folder = "/tmp";
    std::string temp_path = folder;
    temp_path += "/povray_XXXXXX";
    char *scenetemp = &temp_path[0u];
    int scene_fd = mkstemp(scenetemp);
    int scene_chmod = fchmod(scene_fd, 0600);
    std::ostringstream sceneimg;
    if (scene_fd<0||scene_chmod<0) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to create temp file, please check permissions");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    ssize_t scene_s = write(scene_fd, scene.c_str(), scene.size());
    close(scene_fd);
    if (scene_s<0) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to write to temp file, please check permissions");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // Render
    if (fileExists(scenetemp)) {
        int povray = 1;
        std::ostringstream povray_command;
        if (povpath.empty())
            povpath = "povray";
        sceneimg << scenetemp << ".png"; // .hdr // +fh // hdr dont support alpha
        povray_command << povpath << " +I\"" << scenetemp << "\""  << " +O\"" << sceneimg.str() << "\"" << " +FN16 -D0 +H" << height << " +W" << width << " +Q" << quality;
        if (antialiasing>0)
            povray_command << " +A0." << antialiasing;
        if (alpha)
            povray_command << " +UA";
        if (!povinc.empty())
            povray_command << " +L\""+povinc+"\"";
        if (!povcmd.empty())
            povray_command << " " << povcmd;
        const char *deployFolder = getenv("POV_OFX_INCLUDE"); // optional include path for deployment
        if (deployFolder!=0) {
            std::string deployInc = deployFolder;
            povray_command << " +L\""+deployInc+"\"";
        }
        if (startFrame>0&&endFrame>0&&args.time<=endFrame) {
            povray_command << " +KC +KFI" << startFrame << " +KFF" << endFrame << " +SF" << args.time << " +EF" << args.time;
            std::ostringstream lastFrame, currentFrame, prefix;
            lastFrame << endFrame;
            currentFrame << args.time;
            int append = 0;
            if (currentFrame.str().length()<lastFrame.str().length())
                append = lastFrame.str().length()-currentFrame.str().length();
            if (append>0) {
                for(int x=0; x < append ;x++)
                    prefix << 0;
            }
            prefix << args.time;
            sceneimg.str("");
            sceneimg.clear();
            sceneimg << scenetemp << prefix.str() << ".png"; // .hdr // +fh // hdr dont support alpha
        }
        if (system(NULL)) {
            #ifdef DEBUG
            std::cout << "Running POV-Ray using: " << povray_command.str() << std::endl;
            #endif
            povray=system(povray_command.str().c_str());
        }
        (void)unlink(scenetemp);
        if (povray!=0) {
            setPersistentMessage(OFX::Message::eMessageError, "", "POV-Ray failed, please check terminal for more info");
            OFX::throwSuiteStatusException(kOfxStatFailed);
            return;
        }
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Temp scene not found");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // Generate empty image
    Magick::Image image(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,0)"));
    Magick::Image output(Magick::Geometry(width,height),Magick::Color("rgba(0,0,0,1)"));

    // Read render result
    if (fileExists(sceneimg.str())) {
        #ifdef DEBUG
        image.debug(true);
        #endif
        try {
            image.read(sceneimg.str().c_str());
        }
        catch(Magick::Error &error) {
            setPersistentMessage(OFX::Message::eMessageError, "", error.what());
            std::remove(sceneimg.str().c_str());
            int remove_fd = std::remove(sceneimg.str().c_str());
            if (remove_fd!=0) {
                //
            }

            OFX::throwSuiteStatusException(kOfxStatFailed);
            return;
        }
        #ifdef DEBUG
        image.debug(false);
        #endif
        std::remove(sceneimg.str().c_str());
        int remove_fd = std::remove(sceneimg.str().c_str());
        if (remove_fd!=0) {
            //
        }

        image.flip();
        image.colorSpace(Magick::RGBColorspace); // if png file
    }
    else {
        setPersistentMessage(OFX::Message::eMessageError, "", "Temp image not found");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // return image
    if (dstClip_ && dstClip_->isConnected()) {
        output.composite(image, 0, 0, Magick::OverCompositeOp);
        output.composite(image, 0, 0, Magick::CopyOpacityCompositeOp);
        output.write(0,0,args.renderWindow.x2 - args.renderWindow.x1,args.renderWindow.y2 - args.renderWindow.y1,"RGBA",Magick::FloatPixel,(float*)dstImg->getPixelData());
    }
}

void PovRayPlugin::changedParam(const OFX::InstanceChangedArgs &args, const std::string &/*paramName*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    clearPersistentMessage();
}

bool PovRayPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }

    int width,height;
    width_->getValue(width);
    height_->getValue(height);

    if (width>0 && height>0) {
        rod.x1 = rod.y1 = 0;
        rod.x2 = width;
        rod.y2 = height;
    }
    else {
        rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
    }

    return true;
}

mDeclarePluginFactory(PovRayPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void PovRayPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("Persistence of Vision raytracer generic node.\n\nPOV-Ray is not included and must be installed (in PATH) prior to using this node.\n\nhttps://github.com/olear/openfx-arena/wiki/PovRay for more information regarding usage and installation.\n\nPowered by POV-Ray and ImageMagick.\n\nPOV-Ray is Copyright 2003-2015 Persistence of Vision Raytracer Pty. Ltd.\n\nThe terms \"POV-Ray\" and \"Persistence of Vision Raytracer\" are trademarks of Persistence of Vision Raytracer Pty. Ltd.\n\nPOV-Ray is distributed under the AGPL3 license.\n\nImageMagick (R) is Copyright 1999-2015 ImageMagick Studio LLC, a non-profit organization dedicated to making software imaging solutions freely available.\n\nImageMagick is distributed under the Apache 2.0 license.");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    // add other
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    //desc.setHostFrameThreading(kHostFrameThreading);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void PovRayPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{   
    // there has to be an input clip, even for generators
    ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setOptional(true);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    dstClip->setSupportsTiles(kSupportsTiles);

    // make some pages
    PageParamDescriptor *page = desc.definePageParam(kPluginName);
    GroupParamDescriptor *groupCanvas = desc.defineGroupParam("Canvas");
    GroupParamDescriptor *groupAdvanced = desc.defineGroupParam("Advanced");
    groupCanvas->setOpen(false);
    groupAdvanced->setOpen(false);
    {
        page->addChild(*groupCanvas);
        page->addChild(*groupAdvanced);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamScene);
        param->setLabel(kParamSceneLabel);
        param->setHint(kParamSceneHint);
        param->setStringType(eStringTypeMultiLine);
        param->setAnimates(true);
        param->setDefault(kParamSceneDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamWidth);
        param->setLabel(kParamWidthLabel);
        param->setHint(kParamWidthHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamWidthDefault);
        param->setParent(*groupCanvas);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamHeight);
        param->setLabel(kParamHeightLabel);
        param->setHint(kParamHeightHint);
        param->setRange(0, 10000);
        param->setDisplayRange(0, 4000);
        param->setDefault(kParamHeightDefault);
        param->setParent(*groupCanvas);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamQuality);
        param->setLabel(kParamQualityLabel);
        param->setHint(kParamQualityHint);
        param->setRange(0, 11);
        param->setDisplayRange(0, 11);
        param->setDefault(kParamQualityDefault);
        page->addChild(*param);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamAntialiasing);
        param->setLabel(kParamAntialiasingLabel);
        param->setHint(kParamAntialiasingHint);
        param->setRange(0, 9);
        param->setDisplayRange(0, 9);
        param->setDefault(kParamAntialiasingDefault);
        page->addChild(*param);
    }
    {
        BooleanParamDescriptor* param = desc.defineBooleanParam(kParamAlpha);
        param->setLabel(kParamAlphaLabel);
        param->setHint(kParamAlphaHint);
        param->setDefault(kParamAlphaDefault);
        page->addChild(*param);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamPovPath);
        param->setLabel(kParamPovPathLabel);
        param->setHint(kParamPovPathHint);
        param->setStringType(OFX::eStringTypeFilePath);
        param->setDefault(kParamPovPathDefault);
        param->setParent(*groupAdvanced);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamPovInc);
        param->setLabel(kParamPovIncLabel);
        param->setHint(kParamPovIncHint);
        param->setStringType(OFX::eStringTypeDirectoryPath);
        param->setParent(*groupAdvanced);
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamPovCmd);
        param->setLabel(kParamPovCmdLabel);
        param->setHint(kParamPovCmdHint);
        param->setStringType(eStringTypeSingleLine);
        param->setAnimates(true);
        param->setParent(*groupAdvanced);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamSF);
        param->setLabel(kParamSFLabel);
        param->setHint(kParamSFHint);
        param->setRange(0, 100000);
        param->setDisplayRange(0, 1000);
        param->setDefault(kParamSFDefault);
        page->addChild(*param);
        param->setParent(*groupAdvanced);
    }
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamEF);
        param->setLabel(kParamEFLabel);
        param->setHint(kParamEFHint);
        param->setRange(0, 100000);
        param->setDisplayRange(0, 1000);
        param->setDefault(kParamEFDefault);
        page->addChild(*param);
        param->setParent(*groupAdvanced);
    }
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* PovRayPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new PovRayPlugin(handle);
}

static PovRayPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

#endif // _WINDOWS
