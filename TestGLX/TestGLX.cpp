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
#define kPluginGrouping "Arena"
#define kPluginIdentifier "net.fxarena.openfx.TestGLX"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kSupportsTiles 0
#define kSupportsMultiResolution 1
#define kSupportsRenderScale 1
#define kRenderThreadSafety eRenderFullySafe

#define OGL_MAJOR 3
#define OGL_MINOR 0

using namespace OFX;

// GLX
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
    OFX::Clip *srcClip_;
    GLXContext ctx;
    Display* dpy;
    GLXFBConfig* fbc;
};

TestGLXPlugin::TestGLXPlugin(OfxImageEffectHandle handle)
: OFX::ImageEffect(handle)
, dstClip_(0)
, srcClip_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    assert(dstClip_ && dstClip_->getPixelComponents() == OFX::ePixelComponentRGBA);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    assert(srcClip_ && srcClip_->getPixelComponents() == OFX::ePixelComponentRGBA);

    // Setup glx
    static int visual_attribs[] = {
            None
    };
    int context_attribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, OGL_MAJOR,
            GLX_CONTEXT_MINOR_VERSION_ARB, OGL_MINOR,
            None
    };
    dpy = XOpenDisplay(0);
    int fbcount = 0;
    fbc = NULL;

    // Open display
    if (!(dpy = XOpenDisplay(0))){
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to open X11 display");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // Get framebuffer configs
    if (!(fbc = glXChooseFBConfig(dpy,DefaultScreen(dpy),visual_attribs,&fbcount))) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to get framebuffer");
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // Get extensions
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB");
    glXMakeContextCurrentARB = (glXMakeContextCurrentARBProc)glXGetProcAddressARB( (const GLubyte *) "glXMakeContextCurrent");
    if (!(glXCreateContextAttribsARB && glXMakeContextCurrentARB)) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Missing support for GLX_ARB_create_context");
        XFree(fbc);
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // Create a context using glXCreateContextAttribsARB
    if (!( ctx = glXCreateContextAttribsARB(dpy,fbc[0],0,True,context_attribs))) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to create OpenGL context, OpenGL 3.0(+) is needed");
        XFree(fbc);
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }
}

TestGLXPlugin::~TestGLXPlugin()
{
    XFree(fbc);
    XSync(dpy,False);
}

void TestGLXPlugin::render(const OFX::RenderArguments &args)
{
    // render scale
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    // get src clip
    if (!srcClip_) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
    assert(srcClip_);
    std::auto_ptr<const OFX::Image> srcImg(srcClip_->fetchImage(args.time));
    OfxRectI srcRod,srcBounds;
    if (srcImg.get()) {
        srcRod = srcImg->getRegionOfDefinition();
        srcBounds = srcImg->getBounds();
        if (srcImg->getRenderScale().x != args.renderScale.x ||
            srcImg->getRenderScale().y != args.renderScale.y ||
            srcImg->getField() != args.fieldToRender) {
            setPersistentMessage(OFX::Message::eMessageError, "", "OFX Host gave image with wrong scale or field properties");
            OFX::throwSuiteStatusException(kOfxStatFailed);
            return;
        }
    }

    // get dest clip
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

    // get bit depth
    OFX::BitDepthEnum dstBitDepth = dstImg->getPixelDepth();
    if (dstBitDepth != OFX::eBitDepthFloat || (srcImg.get() && (dstBitDepth != srcImg->getPixelDepth()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // get pixel component
    OFX::PixelComponentEnum dstComponents  = dstImg->getPixelComponents();
    if (dstComponents != OFX::ePixelComponentRGBA || (srcImg.get() && (dstComponents != srcImg->getPixelComponents()))) {
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
        return;
    }

    // are we in the image bounds?
    OfxRectI dstBounds = dstImg->getBounds();
    if(args.renderWindow.x1 < dstBounds.x1 || args.renderWindow.x1 >= dstBounds.x2 || args.renderWindow.y1 < dstBounds.y1 || args.renderWindow.y1 >= dstBounds.y2 ||
       args.renderWindow.x2 <= dstBounds.x1 || args.renderWindow.x2 > dstBounds.x2 || args.renderWindow.y2 <= dstBounds.y1 || args.renderWindow.y2 > dstBounds.y2) {
        OFX::throwSuiteStatusException(kOfxStatErrValue);
        return;
    }

    // Setup
    int width = srcRod.x2-srcRod.x1;
    int height = srcRod.y2-srcRod.y1;
    GLXPbuffer pbuf;

    // Create temporary buffer
    int pbuffer_attribs[] = {
        GLX_PBUFFER_WIDTH, width,
        GLX_PBUFFER_HEIGHT, height,
        None
    };
    pbuf = glXCreatePbuffer(dpy,fbc[0],pbuffer_attribs);
    XSync(dpy,False);

    // try to make it the current context
    if (!glXMakeContextCurrent(dpy,pbuf,pbuf,ctx)) {
        if (!glXMakeContextCurrent(dpy,DefaultRootWindow(dpy),DefaultRootWindow(dpy),ctx)) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to make GLX current");
            OFX::throwSuiteStatusException(kOfxStatFailed);
        }
    }

    //Initialize Projection Matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    //Initialize Modelview Matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Initialize clear color
    glClearColor(0.f,0.f,0.f,1.f);

    // Clear color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Disabling the depth test
    glDisable(GL_DEPTH_TEST);

    // Generate a texture
    GLuint mTextureID;
    glGenTextures(1,&mTextureID);

    // Enable texturing
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);

    // Specify texture to use
    glBindTexture(GL_TEXTURE_2D,0);

    // Set texturing parameters
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_FLOAT,srcImg->getPixelData());

    // Quads
    glBegin(GL_QUADS);
    glTexCoord2f(0.f,1.f);
    glVertex2f(-1.0,1.0);
    glTexCoord2f(0.f,0.f);
    glVertex2f(-1.0,-1.0);
    glTexCoord2f(1.f,0.f);
    glVertex2f(1.0,-1.0);
    glTexCoord2f(1.f,1.f);
    glVertex2f(1.0,1.0);
    glEnd();

    // Return
    glReadPixels(0,0,width,height,GL_RGBA,GL_FLOAT,dstImg->getPixelData());
}

bool TestGLXPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
        return false;
    }
    if (srcClip_ && srcClip_->isConnected()) {
        rod = srcClip_->getRegionOfDefinition(args.time);
    } else {
        rod.x1 = rod.y1 = kOfxFlagInfiniteMin;
        rod.x2 = rod.y2 = kOfxFlagInfiniteMax;
    }
    return true;
}

mDeclarePluginFactory(TestGLXPluginFactory, {}, {});

/** @brief The basic describe function, passed a plugin descriptor */
void TestGLXPluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabel(kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription("GLX test node, made by @olear and @johnwo");

    // add the supported contexts
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedContext(eContextFilter);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthFloat);

    desc.setSupportsTiles(kSupportsTiles);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setRenderThreadSafety(kRenderThreadSafety);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
void TestGLXPluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum /*context*/)
{
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);

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
