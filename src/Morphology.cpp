#include "MagickPlugin.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "MorphologyOFX"
#define kPluginGrouping "Extra/Misc"
#define kPluginIdentifier "org.imagemagick.Morphology"
#define kPluginDescription "Morphology modifies an image in various ways based on the nearby neighbourhood of the other pixels that surround it. This in turn can provide a huge range of effects, Shape expansion and contraction (dilate/erode), to distance from edge, to thining down to a skeleton, or mid-line axis. For more information read  https://imagemagick.org/Usage/morphology/#basic"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking true
#define kHostMixing true

#define kParamIterations "iterations"
#define kParamIterationsLabel "Iterations"
#define kParamIterationsHint "Iterations used"
#define kParamIterationsDefault 1

#define kParamMethod "method"
#define kParamMethodLabel "Method"
#define kParamMethodHint "Method used for Morphology. https://imagemagick.org/Usage/morphology/#basic"
#define kParamMethodDefault 3 // Dilate

#define kParamKernel "kernel"
#define kParamKernelLabel "kernel"
#define kParamKernelHint "Kernel used for Morphology. https://imagemagick.org/Usage/morphology/#basic"
#define kParamKernelDefault "Octagon:3"

class MorphologyPlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    MorphologyPlugin(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
        , _iterations(NULL)
        , _method(NULL)
        , _kernel(NULL)
    {
        _iterations = fetchIntParam(kParamIterations);
        _method = fetchChoiceParam(kParamMethod);
        _kernel = fetchStringParam(kParamKernel);
        assert(_iterations && _method && _kernel);
    }

    virtual void render(const OFX::RenderArguments &args,
                        Magick::Image &image) OVERRIDE FINAL
    {
        std::string kernel;
        int methodValue,iterations;
        _iterations->getValueAtTime(args.time, iterations);
        _method->getValueAtTime(args.time, methodValue);
        _kernel->getValueAtTime(args.time, kernel);

        MagickCore::MorphologyMethod method = MagickCore::UndefinedMorphology;
        switch (methodValue) {
        case 0:
            method = MagickCore::ConvolveMorphology;
            break;
        case 1:
            method = MagickCore::CorrelateMorphology;
            break;
        case 2:
            method = MagickCore::ErodeMorphology;
            break;
        case 3:
            method = MagickCore::DilateMorphology;
            break;
        case 4:
            method = MagickCore::ErodeIntensityMorphology;
            break;
        case 5:
            method = MagickCore::DilateIntensityMorphology;
            break;
        case 6:
            method = MagickCore::DistanceMorphology;
            break;
        case 7:
            method = MagickCore::OpenMorphology;
            break;
        case 8:
            method = MagickCore::CloseMorphology;
            break;
        case 9:
            method = MagickCore::OpenIntensityMorphology;
            break;
        case 10:
            method = MagickCore::CloseIntensityMorphology;
            break;
        case 11:
            method = MagickCore::SmoothMorphology;
            break;
        case 12:
            method = MagickCore::EdgeInMorphology;
            break;
        case 13:
            method = MagickCore::EdgeOutMorphology;
            break;
        case 14:
            method = MagickCore::EdgeMorphology;
            break;
        case 15:
            method = MagickCore::TopHatMorphology;
            break;
        case 16:
            method = MagickCore::BottomHatMorphology;
            break;
        case 17:
            method = MagickCore::HitAndMissMorphology;
            break;
        case 18:
            method = MagickCore::ThinningMorphology;
            break;
        case 19:
            method = MagickCore::ThickenMorphology;
            break;
        case 20:
            method = MagickCore::VoronoiMorphology;
            break;
        case 21:
            method = MagickCore::IterativeDistanceMorphology;
            break;
        default:
            method = MagickCore::UndefinedMorphology;
        }
        image.morphology(method, kernel, iterations);
    }
private:
    IntParam *_iterations;
    ChoiceParam *_method;
    StringParam *_kernel;
};

mDeclarePluginFactory(MorphologyPluginFactory, {}, {});

void MorphologyPluginFactory::describe(ImageEffectDescriptor &desc)
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

void MorphologyPluginFactory::describeInContext(ImageEffectDescriptor &desc,
                                                ContextEnum context)
{
    OFX::PageParamDescriptor *page = MorphologyPlugin::describeInContextBegin(desc, context);
    {
        IntParamDescriptor *param = desc.defineIntParam(kParamIterations);
        param->setLabel(kParamIterationsLabel);
        param->setHint(kParamIterationsHint);
        param->setRange(1,100);
        param->setDisplayRange(1, 10);
        param->setDefault(kParamIterationsDefault);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamMethod);
        param->setLabel(kParamMethodLabel);
        param->setHint(kParamMethodHint);
        param->appendOption("Convolve");
        param->appendOption("Correlate");
        param->appendOption("Erode");
        param->appendOption("Dilate");
        param->appendOption("ErodeIntensity");
        param->appendOption("DilateIntensity");
        param->appendOption("Distance");
        param->appendOption("Open");
        param->appendOption("Close");
        param->appendOption("OpenIntensity");
        param->appendOption("CloseIntensity");
        param->appendOption("Smooth");
        param->appendOption("EdgeIn");
        param->appendOption("EdgeOut");
        param->appendOption("Edge");
        param->appendOption("TopHat");
        param->appendOption("BottomHat");
        param->appendOption("HitAndMiss");
        param->appendOption("Thinning");
        param->appendOption("Thicken");
        param->appendOption("Voronoi");
        param->appendOption("IterativeDistance");
        param->setDefault(kParamMethodDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    {
        StringParamDescriptor* param = desc.defineStringParam(kParamKernel);
        param->setLabel(kParamKernelLabel);
        param->setHint(kParamKernelHint);
        param->setStringType(eStringTypeMultiLine);
        param->setDefault(kParamKernelDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    MorphologyPlugin::describeInContextEnd(desc, context, page);
}

ImageEffect* MorphologyPluginFactory::createInstance(OfxImageEffectHandle handle,
                                                     ContextEnum /*context*/)
{
    return new MorphologyPlugin(handle);
}

static MorphologyPluginFactory p(kPluginIdentifier,
                                 kPluginVersionMajor,
                                 kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
