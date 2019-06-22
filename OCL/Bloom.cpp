#include "OCLPlugin.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "Bloom"
#define kPluginGrouping "OpenCL"
#define kPluginIdentifier "net.fxarena.opencl.Bloom"
#define kPluginDescription "Bloom filter using OpenCL."
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamStrength "strength"
#define kParamStrengthLabel "Strength"
#define kParamStrengthHint "Adjust bloom strength"
#define kParamStrengthDefault 0.015

#define kParamSize "size"
#define kParamSizeLabel "Size"
#define kParamSizeHint "Adjust bloom mask size"
#define kParamSizeDefault 5

class BloomCLPlugin
    : public OCLPluginHelper<kSupportsRenderScale>
{
public:
    BloomCLPlugin(OfxImageEffectHandle handle)
        : OCLPluginHelper<kSupportsRenderScale>(handle, "", kPluginIdentifier)
        , _strength(0)
        , _size(0)
    {
        _strength = fetchDoubleParam(kParamStrength);
        _size = fetchDouble2DParam(kParamSize);
        assert(_strength && _size);
    }

    virtual void render(const OFX::RenderArguments &args, cl::Kernel kernel) OVERRIDE FINAL
    {
        double bloomSizeX, bloomSizeY, bloomStrength;
        _strength->getValueAtTime(args.time, bloomStrength);
        _size->getValueAtTime(args.time, bloomSizeX, bloomSizeY);
        kernel.setArg(2, bloomStrength);
        kernel.setArg(3, bloomSizeX);
        kernel.setArg(4, bloomSizeY);
    }
private:
    DoubleParam *_strength;
    Double2DParam *_size;
};

mDeclarePluginFactory(BloomCLPluginFactory, {}, {});

void BloomCLPluginFactory::describe(ImageEffectDescriptor &desc)
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

void BloomCLPluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    OFX::PageParamDescriptor *page = BloomCLPlugin::describeInContextBegin(desc, context);
    {
        DoubleParamDescriptor *param = desc.defineDoubleParam(kParamStrength);
        param->setLabel(kParamStrengthLabel);
        param->setHint(kParamStrengthHint);
        param->setRange(0, 1000);
        param->setDisplayRange(0, 0.1);
        param->setDefault(kParamStrengthDefault);
        if (page) {
            page->addChild(*param);
        }

    }
    {
        Double2DParamDescriptor *param = desc.defineDouble2DParam(kParamSize);
        param->setLabel(kParamSizeLabel);
        param->setHint(kParamSizeHint);
        param->setRange(0, 0, 1000, 1000);
        param->setDisplayRange(0, 0, 50, 50);
        param->setDefault(kParamSizeDefault, kParamSizeDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    BloomCLPlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
BloomCLPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new BloomCLPlugin(handle);
}

static BloomCLPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
