#include "OCLPlugin.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "VibranceCL"
#define kPluginGrouping "Extra/OpenCL"
#define kPluginIdentifier "net.fxarena.opencl.Vibrance"
#define kPluginDescription "Vibrance filter using OpenCL."
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamAmount "Amount"
#define kParamAmountLabel "Amount"
#define kParamAmountHint "Adjust effect amount"
#define kParamAmountDefault 0.5

class VibranceCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    VibranceCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle, "", kPluginIdentifier)
        , _amount(0)
    {
        _amount = fetchDoubleParam(kParamAmount);
        assert(_amount);
    }

    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) OVERRIDE FINAL
    {
        double effectAmount;
        _amount->getValueAtTime(args.time, effectAmount);
        kernel.setArg(2, effectAmount);
    }
private:
    DoubleParam *_amount;
};

mDeclarePluginFactory(VibranceCLPluginFactory, {}, {});

void VibranceCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void VibranceCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = VibranceCLPlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamAmount);
        param->setLabel(kParamAmountLabel);
        param->setHint(kParamAmountHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 10);
        param->setDefault(kParamAmountDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    VibranceCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
VibranceCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new VibranceCLPlugin(handle);
}

static VibranceCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
