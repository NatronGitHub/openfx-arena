/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "TestGLX.h"
#include "ofxsMacros.h"
#include <iostream>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#define kPluginName "TestGLX"
#define kPluginGrouping "Other/Test"

#define kPluginIdentifier "net.fxarena.openfx.TestGLX"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 0
#define kRenderThreadSafety eRenderFullySafe

using namespace OFX;
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*,GLXFBConfig,GLXContext,Bool,const int*);
typedef Bool (*glXMakeContextCurrentARBProc)(Display*,GLXDrawable,GLXDrawable,GLXContext);
static glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
static glXMakeContextCurrentARBProc glXMakeContextCurrentARB = 0;

class TestGLXPlugin : public OFX::ImageEffect
{
public:
    TestGLXPlugin(OfxImageEffectHandle handle);
    virtual ~TestGLXPlugin();
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;
private:
    OFX::Clip *dstClip_;
};

TestGLXPlugin::TestGLXPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && (dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA || dstClip_->getPixelComponents() == OFX::ePixelComponentRGB));
}

TestGLXPlugin::~TestGLXPlugin()
{
}

/* Override the render */
void TestGLXPlugin::render(const OFX::RenderArguments &args)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (!dstClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(dstClip_);

    std::auto_ptr<OFX::Image> dstImg(dstClip_->fetchImage(args.time));
    if (!dstImg.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (dstImg->getRenderScale().x != args.renderScale.x ||
        dstImg->getRenderScale().y != args.renderScale.y ||
        dstImg->getField() != args.fieldToRender) {
        setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat && dstBitDepth != OFX::eBitDepthUShort && dstBitDepth != OFX::eBitDepthUByte) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if ((dstComponents != OFX::ePixelComponentRGBA && dstComponents != OFX::ePixelComponentRGB && dstComponents != OFX::ePixelComponentAlpha)) {
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

    // image size
    int width = dstRod.x2-dstRod.x1;
    int height = dstRod.y2-dstRod.y1;

    // setup glx
    static int visual_attribs[] = {
            None
    };
    int context_attribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 0,
            None
    };

    Display* dpy = XOpenDisplay(0);
    int fbcount = 0;
    GLXFBConfig* fbc = NULL;
    GLXContext ctx;
    GLXPbuffer pbuf;

    // open display
    if (!(dpy = XOpenDisplay(0))){
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to open X11 display");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // get framebuffer configs
    if (!(fbc = glXChooseFBConfig(dpy,DefaultScreen(dpy),visual_attribs,&fbcount))) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to get framebuffer");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // get extensions
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB");
    glXMakeContextCurrentARB = (glXMakeContextCurrentARBProc)glXGetProcAddressARB( (const GLubyte *) "glXMakeContextCurrent");
    if (!(glXCreateContextAttribsARB && glXMakeContextCurrentARB)) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Missing support for GLX_ARB_create_context");
        XFree(fbc);
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // create a context using glXCreateContextAttribsARB
    if (!( ctx = glXCreateContextAttribsARB(dpy,fbc[0],0,True,context_attribs))) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to create OpenGL context");
        XFree(fbc);
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // create temporary buffer
    int pbuffer_attribs[] = {
        GLX_PBUFFER_WIDTH, width,
        GLX_PBUFFER_HEIGHT, height,
        None
    };
    pbuf = glXCreatePbuffer(dpy,fbc[0],pbuffer_attribs);
    XFree(fbc);
    XSync(dpy,False);

    // try to make it the current context
    if (!glXMakeContextCurrent(dpy,pbuf,pbuf,ctx)) {
        if (!glXMakeContextCurrent(dpy,DefaultRootWindow(dpy),DefaultRootWindow(dpy),ctx)) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to make GLX current");
            OFX::throwSuiteStatusException(kOfxStatFailed);
        }
    }

    //std::cout << "using OpenGL " << glGetString(GL_VERSION) << std::endl;

    //Initialize Projection Matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //Initialize Modelview Matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Initialize clear color
    glClearColor(0.f,0.f,0.f,1.f);

    //Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    //Render something
    glBegin(GL_QUADS);
        glVertex2f(-0.5f,-0.5f);
        glVertex2f(0.5f,-0.5f);
        glVertex2f(0.5f,0.5f);
        glVertex2f(-0.5f,0.5f);
    glEnd();

    // output
    float *pixels;
    float *output;
    output = (float*)dstImg->getPixelData();
    pixels = new float[width*height*4];
    for(int i=0; i <(width*height*4); i++ )
        pixels[i]=0;
    glReadPixels(0,0,width,height,GL_RGBA,GL_FLOAT,pixels);
    for(int i=0; i <(width*height*4); i++ )
        output[i]=pixels[i];
}

bool TestGLXPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }

    rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
    rod.x2 = rod.y2 = kOfxFlagInfiniteMax;

    return true;
}

mDeclarePluginFactory(TestGLXPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void TestGLXPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("GLX (OpenGL 3.0+) test node, draws a white rectangle");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextGenerator);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TestGLXPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
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
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
ImageEffect* TestGLXPluginFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new TestGLXPlugin(handle);
}

void getTestGLXPluginID(OFX::PluginFactoryArray &ids)
{
    static TestGLXPluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
