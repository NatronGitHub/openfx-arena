// WORK IN PROGRESS

#include "ArenaIO.h"
#include <iostream>
#include <stdint.h>
#include <Magick++.h>
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "ofxsMacros.h"
#ifdef OFX_IO_USING_OCIO
#include <OpenColorIO/OpenColorIO.h>
#endif

#define kPluginName "ArenaIO"
#define kPluginGrouping "Image/Readers"
#define kPluginDescription "Read various image format using ImageMagick"
#define kPluginIdentifier "net.fxarena.openfx.ArenaIO"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsAlpha false
#define kSupportsTiles false

class ArenaIOPlugin : public GenericReaderPlugin
{
public:
    ArenaIOPlugin(OfxImageEffectHandle handle);
    virtual ~ArenaIOPlugin();
private:
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, const OfxRectI& renderWindow, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, OfxRectI *bounds, double *par, std::string *error) OVERRIDE FINAL;
    virtual void onInputFileChanged(const std::string& newFile, OFX::PreMultiplicationEnum *premult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
};

ArenaIOPlugin::ArenaIOPlugin(OfxImageEffectHandle handle)
: GenericReaderPlugin(handle, kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles, false)
{
    Magick::InitializeMagick(NULL);
}

ArenaIOPlugin::~ArenaIOPlugin()
{
}

void
ArenaIOPlugin::decode(const std::string& filename,
                      OfxTime /*time*/,
                      const OfxRectI& renderWindow,
                      float *pixelData,
                      const OfxRectI& bounds,
                      OFX::PixelComponentEnum pixelComponents,
                      int pixelComponentCount,
                      int rowBytes)
{
    Magick::Image image;
    image.read(filename.c_str());
    if (image.columns() && image.rows()) {
        if (!image.matte())
            image.matte(true);
        if (image.depth()<32)
            image.depth(32);
        image.flip();
        image.write(0,0,bounds.x2,bounds.y2,"RGBA",Magick::FloatPixel,pixelData);
    }
}

bool ArenaIOPlugin::getFrameBounds(const std::string& filename,
                              OfxTime /*time*/,
                              OfxRectI *bounds,
                              double *par,
                              std::string *error)
{
    assert(bounds);
    Magick::Image image;
    image.read(filename.c_str());
    if (image.columns()>0 && image.rows()>0) {
        bounds->x1 = 0;
        bounds->x2 = image.columns();
        bounds->y1 = 0;
        bounds->y2 = image.rows();
        *par = 1.0;
        return true;
    }
    return false;
}

void ArenaIOPlugin::onInputFileChanged(const std::string& /*newFile*/,
                                  OFX::PreMultiplicationEnum *premult,
                                  OFX::PixelComponentEnum *components,int *componentCount)
{
    assert(premult && components);
    *components = OFX::ePixelComponentRGBA;
    *premult = OFX::eImageOpaque;
}

using namespace OFX;

mDeclareReaderPluginFactory(ArenaIOPluginFactory, {}, {}, false);

/** @brief The basic describe function, passed a plugin descriptor */
void ArenaIOPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, kSupportsTiles, false);
    desc.setLabel(kPluginName);

    #ifdef OFX_EXTENSIONS_TUTTLE
    const char* extensions[] = {"xcf", "psd", NULL};
    desc.addSupportedExtensions(extensions);
    desc.setPluginEvaluation(10);
    #endif

    desc.setPluginDescription(kPluginDescription);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void ArenaIOPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsAlpha, kSupportsTiles);
    GenericReaderDescribeInContextEnd(desc, context, page, "reference", "reference");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* ArenaIOPluginFactory::createInstance(OfxImageEffectHandle handle,
                                     ContextEnum /*context*/)
{
    ArenaIOPlugin* ret =  new ArenaIOPlugin(handle);
    ret->restoreStateFromParameters();
    return ret;
}

void getArenaIOPluginID(OFX::PluginFactoryArray &ids)
{
    static ArenaIOPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
