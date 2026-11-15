/*
 * openfx-arena <https://github.com/rodlie/openfx-arena>,
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "SwirlOFX"
#define kPluginGrouping "Extra/Distort"
#define kPluginIdentifier "net.fxarena.openfx.Swirl"
#define kPluginDescription "Swirl effect using ImageMagick."
#define kPluginVersionMajor 3 // 3: unpremultiply the source before ImageMagick (see magickReadRGBA)
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamSwirl "amount"
#define kParamSwirlLabel "Amount"
#define kParamSwirlHint "Swirl amount."
#define kParamSwirlDefault 60

class SwirlPlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    SwirlPlugin(OfxImageEffectHandle handle, bool unpremultSrc)
        : MagickPluginHelper<kSupportsRenderScale>(handle, unpremultSrc)
        , _swirl(NULL)
    {
        _swirl = fetchDoubleParam(kParamSwirl);
        assert(_swirl);
    }

    virtual void render(const OFX::RenderArguments &args, Magick::Image &image) OVERRIDE FINAL
    {
        double amount;
        _swirl->getValueAtTime(args.time, amount);
        image.swirl(amount);
    }
private:
    DoubleParam *_swirl;
};

mDeclarePluginFactoryVersioned(SwirlPluginFactory, {}, {});

template<unsigned int majorVersion>
void SwirlPluginFactory<majorVersion>::describe(ImageEffectDescriptor &desc)
{
    if (majorVersion < kPluginVersionMajor) {
        desc.setIsDeprecated(true);
    }
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

template<unsigned int majorVersion>
void SwirlPluginFactory<majorVersion>::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = SwirlPlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamSwirl);
        param->setLabel(kParamSwirlLabel);
        param->setHint(kParamSwirlHint);
        param->setRange(-360, 360);
        param->setDisplayRange(-360, 360);
        param->setDefault(kParamSwirlDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    SwirlPlugin::describeInContextEnd(desc, context, page);
}

template<unsigned int majorVersion>
OFX::ImageEffect*
SwirlPluginFactory<majorVersion>::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    // versions before 3 fed premultiplied pixels to ImageMagick: keep their output
    return new SwirlPlugin(handle, majorVersion >= 3);
}

static SwirlPluginFactory<kPluginVersionMajor> p(kPluginIdentifier, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)
// version 2, kept so that existing projects render as before
static SwirlPluginFactory<2> p2(kPluginIdentifier, 9);
mRegisterPluginFactoryInstance(p2)

OFXS_NAMESPACE_ANONYMOUS_EXIT
