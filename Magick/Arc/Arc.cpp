/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * openfx-arena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
*/

#include "MagickPlugin.h"
#include <iostream>
#include <cmath>

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "ArcOFX"
#define kPluginGrouping "Extra/Distort"
#define kPluginIdentifier "net.fxarena.openfx.Arc"
#define kPluginDescription "Arc effect using ImageMagick."
#define kPluginVersionMajor 4
#define kPluginVersionMinor 9

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamArcAngle "angle"
#define kParamArcAngleLabel "Angle"
#define kParamArcAngleHint "Arc angle"
#define kParamArcAngleDefault 60

#define kParamArcRotate "rotate"
#define kParamArcRotateLabel "Rotate"
#define kParamArcRotateHint "Arc rotate"
#define kParamArcRotateDefault 0

#define kParamArcTopRadius "top"
#define kParamArcTopRadiusLabel "Top radius"
#define kParamArcTopRadiusHint "Arc top radius"
#define kParamArcTopRadiusDefault 0

#define kParamArcBottomRadius "bottom"
#define kParamArcBottomRadiusLabel "Bottom radius"
#define kParamArcBottomRadiusHint "Arc bottom radius"
#define kParamArcBottomRadiusDefault 0

#define kParamFlip "flip"
#define kParamFlipLabel "Flip"
#define kParamFlipHint "Flip image"
#define kParamFlipDefault false

class ArcPlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    ArcPlugin(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
        , _angle(0)
        , _rotate(0)
        , _top(0)
        , _bottom(0)
        , _flip(0)
    {
        _angle = fetchDoubleParam(kParamArcAngle);
        _rotate = fetchDoubleParam(kParamArcRotate);
        _top = fetchDoubleParam(kParamArcTopRadius);
        _bottom = fetchDoubleParam(kParamArcBottomRadius);
        _flip = fetchBooleanParam(kParamFlip);
        assert(_angle && _rotate && _top && _bottom && _flip);
    }

    virtual void render(const OFX::RenderArguments &args, Magick::Image &image) OVERRIDE FINAL
    {
        int width = (int)image.columns();
        int height = (int)image.rows();

        double angle,rotate,topRadius,bottomRadius;
        bool flip = false;
        _angle->getValueAtTime(args.time, angle);
        _rotate->getValueAtTime(args.time, rotate);
        _top->getValueAtTime(args.time,topRadius);
        _bottom->getValueAtTime(args.time, bottomRadius);
        _flip->getValueAtTime(args.time, flip);

        double topRadiusRenderScale = std::floor(topRadius * args.renderScale.x + 0.5);
        double bottomRadiusRenderScale = std::floor(bottomRadius * args.renderScale.x + 0.5);

        int numArgs=4;
        if (topRadius==0 || bottomRadius==0) {
            numArgs=2;
        }

        const double distortArgs1[2] = {angle, rotate};
        const double distortArgs2[4] = {angle, rotate, topRadiusRenderScale, bottomRadiusRenderScale};

        image.backgroundColor(Magick::Color("rgba(0,0,0,0)"));

        if (flip) {
            image.flip();
        }
        if (numArgs==2) {
            image.distort(Magick::ArcDistortion, numArgs, distortArgs1, Magick::MagickTrue);
        } else if (numArgs==4) {
            image.distort(Magick::ArcDistortion, numArgs, distortArgs2, Magick::MagickTrue);
        }
        if (flip) {
            image.flip();
        }

        std::ostringstream scaleW;
        scaleW << width << "x";
        std::ostringstream scaleH;
        scaleH << "x" << height;
        std::size_t columns = width;
        std::size_t rows = height;
        if (image.columns() > columns) {
            image.scale(scaleW.str());
        }
        if (image.rows() > rows) {
            image.scale(scaleH.str());
        }
        image.extent(Magick::Geometry(width, height), Magick::CenterGravity);
    }
private:
    OFX::DoubleParam *_angle;
    OFX::DoubleParam *_rotate;
    OFX::DoubleParam *_top;
    OFX::DoubleParam *_bottom;
    OFX::BooleanParam *_flip;
};

mDeclarePluginFactory(ArcPluginFactory, {}, {});

void ArcPluginFactory::describe(ImageEffectDescriptor &desc)
{
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);
    desc.addSupportedBitDepth(eBitDepthFloat);
    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
    desc.setHostFrameThreading(kHostFrameThreading);
    desc.setHostMaskingEnabled(kHostMasking);
    desc.setHostMixingEnabled(kHostMixing);
}

void ArcPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = ArcPlugin::describeInContextBegin(desc, context);
    {
        {
            DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcAngle);
            param->setLabel(kParamArcAngleLabel);
            param->setHint(kParamArcAngleHint);
            param->setRange(1, 360);
            param->setDisplayRange(1, 360);
            param->setDefault(kParamArcAngleDefault);
            if (page) {
                page->addChild(*param);
            }
        }
        {
            DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcRotate);
            param->setLabel(kParamArcRotateLabel);
            param->setHint(kParamArcRotateHint);
            param->setRange(0, 360);
            param->setDisplayRange(0, 360);
            param->setDefault(kParamArcRotateDefault);
            if (page) {
                page->addChild(*param);
            }
        }
        {
            DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcTopRadius);
            param->setLabel(kParamArcTopRadiusLabel);
            param->setHint(kParamArcTopRadiusHint);
            param->setRange(0, 700);
            param->setDisplayRange(0, 700);
            param->setDefault(kParamArcTopRadiusDefault);
            if (page) {
                page->addChild(*param);
            }
        }
        {
            DoubleParamDescriptor *param = desc.defineDoubleParam(kParamArcBottomRadius);
            param->setLabel(kParamArcBottomRadiusLabel);
            param->setHint(kParamArcBottomRadiusHint);
            param->setRange(0, 350);
            param->setDisplayRange(0, 350);
            param->setDefault(kParamArcBottomRadiusDefault);
            if (page) {
                page->addChild(*param);
            }
        }
        {
            BooleanParamDescriptor *param = desc.defineBooleanParam(kParamFlip);
            param->setLabel(kParamFlipLabel);
            param->setHint(kParamFlipHint);
            param->setDefault(kParamFlipDefault);
            param->setAnimates(false);
            if (page) {
                page->addChild(*param);
            }
        }
    }
    ArcPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
ArcPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new ArcPlugin(handle);
}

static ArcPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
