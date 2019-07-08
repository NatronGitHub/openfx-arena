#include "MagickPlugin.h"

using namespace OFX;
OFXS_NAMESPACE_ANONYMOUS_ENTER

#define kPluginName "CompareOFX"
#define kPluginGrouping "Extra/Misc"
#define kPluginIdentifier "org.imagemagick.Compare"
#define kPluginDescription "Visually annotate the difference between an image and its reconstruction."
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe
#define kHostFrameThreading false
#define kHostMasking false
#define kHostMixing false

#define kOfxImageEffectCompareClipName "Compare"

#define kParamMetric "metric"
#define kParamMetricLabel "Metric"
#define kParamMetricHint "Metric type used"
#define kParamMetricDefault 0 // AbsoluteErrorMetric

class ComparePlugin
    : public MagickPluginHelper<kSupportsRenderScale>
{
public:
    ComparePlugin(OfxImageEffectHandle handle)
        : MagickPluginHelper<kSupportsRenderScale>(handle)
        , _compareClip(NULL)
        , _metric(NULL)
    {
        _compareClip = fetchClip(kOfxImageEffectCompareClipName);
        assert(_compareClip && _compareClip->getPixelComponents() == OFX::ePixelComponentRGBA);

        _metric = fetchChoiceParam(kParamMetric);
        assert(_metric);
    }

    virtual void render(const OFX::RenderArguments &args,
                        Magick::Image &image) OVERRIDE FINAL
    {
        if (_compareClip && _compareClip->isConnected()) {
            OFX::auto_ptr<const OFX::Image> extImg(_compareClip->fetchImage(args.time));
            if (extImg.get()) {
                OfxRectI extBounds = extImg->getBounds();
                if (extImg->getRenderScale().x != args.renderScale.x ||
                    extImg->getRenderScale().y != args.renderScale.y ||
                    ((extImg->getField() != OFX::eFieldNone) && (extImg->getField() != args.fieldToRender))) {
                    setPersistentMessage(OFX::Message::eMessageError,
                                         "",
                                         "OFX Host gave image with wrong scale or field properties");
                    OFX::throwSuiteStatusException(kOfxStatFailed);
                    return;
                }
                int extWidth = extBounds.x2 - extBounds.x1;
                int extHeight = extBounds.y2 - extBounds.y1;
                try {
                    Magick::Image image2(Magick::Geometry(extWidth, extHeight),
                                        Magick::Color("rgba(0,0,0,0)"));
                    // be quiet?
#ifdef DEBUG
                    image2.quiet(false);
#else
                    image2.quiet(true);
#endif
                    image2.read(extWidth,
                                extHeight,
                                "RGBA",
                                Magick::FloatPixel,
                                (float*)extImg->getPixelData());
                    image2.flip(); // always flip

                    // compare images and render difference
                    int metricValue = 0;
                    _metric->getValueAtTime(args.time, metricValue);
                    MagickCore::MetricType metric = MagickCore::AbsoluteErrorMetric;
                    double distortion = 0.0;
                    switch (metricValue) { // set metric
                    case 0:
                        metric = MagickCore::AbsoluteErrorMetric;
                        break;
                    case 1:
                        metric = MagickCore::MeanAbsoluteErrorMetric;
                        break;
                    case 2:
                        metric = MagickCore::MeanSquaredErrorMetric;
                        break;
                    case 3:
                        metric = MagickCore::PeakAbsoluteErrorMetric;
                        break;
                    case 4:
                        metric = MagickCore::RootMeanSquaredErrorMetric;
                        break;
                    case 5:
                        metric = MagickCore::NormalizedCrossCorrelationErrorMetric;
                        break;
                    case 6:
                        metric = MagickCore::FuzzErrorMetric;
                        break;
                    case 7:
                        metric = MagickCore::PerceptualHashErrorMetric;
                        break;
                    default:
                        metric = MagickCore::UndefinedErrorMetric;
                    }
                    image = image.compare(image2, metric, &distortion); // compare
                }
                catch(Magick::Warning &warning) { // show warning
                    setPersistentMessage(OFX::Message::eMessageWarning, "", warning.what());
                }
                catch(Magick::Error &error) { // show error
                    setPersistentMessage(OFX::Message::eMessageError, "", error.what());
                    throwSuiteStatusException(kOfxStatFailed);
                }
            } else { // failed to read image
                throwSuiteStatusException(kOfxStatFailed);
            }
        } // if _compareClip && _compareClip->isConnected()
    }

private:
    Clip *_compareClip;
    ChoiceParam *_metric;
};

mDeclarePluginFactory(ComparePluginFactory, {}, {});

void ComparePluginFactory::describe(ImageEffectDescriptor &desc)
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

void ComparePluginFactory::describeInContext(ImageEffectDescriptor &desc, ContextEnum context)
{
    ClipDescriptor *compareClip = desc.defineClip(kOfxImageEffectCompareClipName);
    compareClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    compareClip->setTemporalClipAccess(false);
    compareClip->setSupportsTiles(false);
    compareClip->setIsMask(false);
    compareClip->setOptional(false);

    PageParamDescriptor *page = ComparePlugin::describeInContextBegin(desc, context);
    {
        OFX::ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamMetric);
        param->setLabel(kParamMetricLabel);
        param->setHint(kParamMetricHint);
        param->appendOption("Absolute Error");
        param->appendOption("Mean Absolute Error");
        param->appendOption("Mean Squared Error");
        param->appendOption("Peak Absolute Error");
        param->appendOption("Root Mean Squared Error");
        param->appendOption("Normalized Cross Correlation Error");
        param->appendOption("Fuzz Error");
        param->appendOption("Perceptual Hash Error");
        param->setDefault(kParamMetricDefault);
        param->setAnimates(false);
        if (page) {
            page->addChild(*param);
        }
    }
    ComparePlugin::describeInContextEnd(desc, context, page);
}

OFX::ImageEffect*
ComparePluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new ComparePlugin(handle);
}

static ComparePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
mRegisterPluginFactoryInstance(p)

OFXS_NAMESPACE_ANONYMOUS_EXIT
