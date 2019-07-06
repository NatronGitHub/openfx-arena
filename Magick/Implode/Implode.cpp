#include "MagickPlugin.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "ImplodeOFX"
#define kPluginGrouping "Extra/Distort"
#define kPluginIdentifier "net.fxarena.openfx.Implode"
#define kPluginDescription "Implode transform node."
#define kPluginVersionMajor 2
#define kPluginVersionMinor 4

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamImplode "factor"
#define kParamImplodeLabel "Factor"
#define kParamImplodeHint "Implode image by factor"
#define kParamImplodeDefault 0.5

#define kParamSwirl "swirl"
#define kParamSwirlLabel "Swirl"
#define kParamSwirlHint "Swirl image by degree"
#define kParamSwirlDefault 0

class ImplodePlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    ImplodePlugin(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
        , _swirl(NULL)
        , _factor(NULL)
    {
        _swirl = fetchDoubleParam(kParamSwirl);
        _factor = fetchDoubleParam(kParamImplode);
        assert(_swirl && _factor);
    }

    virtual void render(const OFX::RenderArguments &args, Magick::Image &image) OVERRIDE FINAL
    {
        double degree, factor;
        _swirl->getValueAtTime(args.time, degree);
        _factor->getValueAtTime(args.time, factor);
        image.implode(factor);
        if (degree != 0) {
            image.swirl(degree);
        }
    }
private:
    DoubleParam *_swirl;
    DoubleParam *_factor;
};

mDeclarePluginFactory(ImplodePluginFactory, {}, {});

void ImplodePluginFactory::describe(ImageEffectDescriptor &desc)
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

void ImplodePluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = ImplodePlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamImplode);
        param->setLabel(kParamImplodeLabel);
        param->setHint(kParamImplodeHint);
        param->setRange(-100, 100);
        param->setDisplayRange(-5, 5);
        param->setDefault(kParamImplodeDefault);
        page->addChild(*param);
    }
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
    ImplodePlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
ImplodePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new ImplodePlugin(handle);
}

static ImplodePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
