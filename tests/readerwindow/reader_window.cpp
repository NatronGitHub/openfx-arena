/*
 * reader_window: render a reader with render windows smaller than its image (#105), or partly
 * or entirely outside of it (#108).
 *
 *   reader_window <identifier> <major> [name=value...] [--scale s] [--window x1,y1,x2,y2]...
 *
 * A small OFX host built on openfx's HostSupport and hostDemo's host classes (hostDemo itself
 * can't do this: its --window renders the whole RoD for a plug-in without tiles, as the readers
 * are). The OFX spec only says that a plug-in without tiles gets full RoD images
 * (kOfxImageEffectPropSupportsTiles: "If a clip or plugin does not support tiled images, then the
 * host should supply full RoD images to the effect whenever it fetches one"), not that the render
 * window is the whole RoD, so this host does what such a host may do: the output image covers
 * the RoD, and the render window is a part of it.
 *
 * It creates the plug-in in the general context, sets the parameters (string, integer, choice
 * index, double, boolean), renders t=0 with the whole RoD as the render window (the reference),
 * then renders each window (by default: the middle quarter of the image, a few windows at its
 * edges, offset from the origin, and a few windows partly or entirely outside of the RoD) into a
 * new image, and checks that
 *   - the render succeeds,
 *   - the pixels of the window that are in the RoD are bit-identical to the same pixels of the
 *     reference,
 *   - the pixels of the window that are outside of the RoD are transparent black (0): the OFX
 *     spec says "If an effect is asked to render outside of its region of definition, it should
 *     fill those pixels in with black transparent pixels" (ofxRendering.rst),
 *   - the pixels outside of the window still hold hostDemo's sentinel (NaN): the reader wrote
 *     only the render window.
 * The output image covers the RoD and the window (a host must give an output image that covers
 * the render window). --scale renders everything at that render scale (default: 1), e.g. 0.5 as
 * a 50% viewer or proxy mode would: the RoD and the windows are then in pixels at that scale.
 * The output depth is float. OFX_PLUGIN_PATH must contain the bundle (no standard paths, no
 * plug-in cache) and OCIO must be set, as for hostDemo. Exit status: 0 if every check passes.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ofxCore.h"
#include "ofxImageEffect.h"
#ifdef OFX_EXTENSIONS_NUKE
#include "nuke/fnOfxExtensions.h"
#endif

#include "ofxhBinary.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhMemory.h"
#include "ofxhImageEffect.h"
#include "ofxhPluginAPICache.h"
#include "ofxhPluginCache.h"
#include "ofxhHost.h"
#include "ofxhImageEffectAPI.h"

#include "hostDemoHostDescriptor.h"
#include "hostDemoEffectInstance.h"
#include "hostDemoClipInstance.h"

namespace {

bool
parseInts(const std::string& s, std::vector<int>& v)
{
    v.clear();
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        char* end = NULL;
        const long l = std::strtol(item.c_str(), &end, 10);
        if (item.empty() || *end != '\0') {
            return false;
        }
        v.push_back((int)l);
    }
    return true;
}

// set a parameter from "name=value", then send the instance changed actions
bool
setParam(OFX::Host::ImageEffect::Instance* instance, const std::string& assignment)
{
    const std::string::size_type eq = assignment.find('=');
    if (eq == std::string::npos || eq == 0) {
        std::fprintf(stderr, "Error: expected name=value, got \"%s\"\n", assignment.c_str());
        return false;
    }
    const std::string name = assignment.substr(0, eq);
    const std::string value = assignment.substr(eq + 1);
    OFX::Host::Param::Instance* p = instance->getParam(name);
    if (!p) {
        std::fprintf(stderr, "Error: no parameter \"%s\"\n", name.c_str());
        return false;
    }
    OfxStatus st = kOfxStatErrUnsupported;
    std::vector<int> iv;
    if (OFX::Host::Param::StringInstance* s = dynamic_cast<OFX::Host::Param::StringInstance*>(p)) {
        st = s->set(value.c_str());
    } else if (OFX::Host::Param::IntegerInstance* i = dynamic_cast<OFX::Host::Param::IntegerInstance*>(p)) {
        st = (parseInts(value, iv) && iv.size() == 1) ? i->set(iv[0]) : kOfxStatErrValue;
    } else if (OFX::Host::Param::ChoiceInstance* c = dynamic_cast<OFX::Host::Param::ChoiceInstance*>(p)) {
        st = (parseInts(value, iv) && iv.size() == 1) ? c->set(iv[0]) : kOfxStatErrValue;
    } else if (OFX::Host::Param::BooleanInstance* b = dynamic_cast<OFX::Host::Param::BooleanInstance*>(p)) {
        st = (value == "1" || value == "true") ? b->set(true) : (value == "0" || value == "false") ? b->set(false) : kOfxStatErrValue;
    } else if (OFX::Host::Param::DoubleInstance* d = dynamic_cast<OFX::Host::Param::DoubleInstance*>(p)) {
        char* end = NULL;
        const double v = std::strtod(value.c_str(), &end);
        st = (!value.empty() && *end == '\0') ? d->set(v) : kOfxStatErrValue;
    }
    if (st != kOfxStatOK) {
        std::fprintf(stderr, "Error: could not set %s (status %d)\n", assignment.c_str(), (int)st);
        return false;
    }
    OfxPointD scale;
    scale.x = scale.y = 1.;
    instance->beginInstanceChangedAction(kOfxChangeUserEdited);
    const OfxStatus st2 = instance->paramInstanceChangedAction(name, kOfxChangeUserEdited, 0., scale);
    instance->endInstanceChangedAction(kOfxChangeUserEdited);
    if (st2 != kOfxStatOK && st2 != kOfxStatReplyDefault) {
        std::fprintf(stderr, "Error: instance changed action failed for %s (status %d)\n", name.c_str(), (int)st2);
        return false;
    }
    return true;
}

// the smallest rectangle that contains a and b
OfxRectI
unionRect(const OfxRectI& a, const OfxRectI& b)
{
    OfxRectI r;
    r.x1 = a.x1 < b.x1 ? a.x1 : b.x1;
    r.y1 = a.y1 < b.y1 ? a.y1 : b.y1;
    r.x2 = a.x2 > b.x2 ? a.x2 : b.x2;
    r.y2 = a.y2 > b.y2 ? a.y2 : b.y2;
    return r;
}

// render t=0 (view 0) at renderScale with the given render window, into a new output image that
// covers the RoD and the window. Returns the output image (owned by the clip until the next
// render), or NULL.
MyHost::MyImage*
render(MyHost::MyEffectInstance* instance, MyHost::MyClipInstance* outputClip, const OfxPointD& renderScale, const OfxRectI& rod, const OfxRectI& window)
{
    // the output image covers the render state's window: the RoD (a full RoD image, as for a
    // plug-in without tiles), and the render window if it goes out of the RoD, whatever the
    // render window passed to the render action
    std::map<OFX::Host::ImageEffect::ClipInstance*, OfxRectD> rois;
    instance->setRenderState(renderScale, unionRect(rod, window), rois);
    outputClip->releaseOutputImage();
    const int imageErrors = MyHost::gImageErrors;
#ifdef OFX_EXTENSIONS_NUKE
    std::list<std::string> planes;
    planes.push_back(kFnOfxImagePlaneColour);
#endif
    const OfxStatus stat = instance->renderAction(0., kOfxImageFieldNone, window, renderScale,
                                                  /*sequentialRender=*/false, /*interactiveRender=*/false,
#ifdef OFX_SUPPORTS_OPENGLRENDER
                                                  /*openGLRender=*/false,
#  ifdef OFX_EXTENSIONS_NATRON
                                                  /*contextData=*/NULL,
#  endif
#endif
                                                  /*draftRender=*/false
#if defined(OFX_EXTENSIONS_VEGAS) || defined(OFX_EXTENSIONS_NUKE)
                                                  , /*view=*/0
#endif
#ifdef OFX_EXTENSIONS_VEGAS
                                                  , /*nViews=*/1
#endif
#ifdef OFX_EXTENSIONS_NUKE
                                                  , planes
#endif
                                                  );
    instance->clearRenderState();
    MyHost::MyImage* image = outputClip->getOutputImage();
    if (stat != kOfxStatOK || !image || MyHost::gImageErrors != imageErrors) {
        std::fprintf(stderr, "Error: render of (%d,%d,%d,%d) failed (status %d%s)\n", window.x1, window.y1, window.x2, window.y2,
                     (int)stat, image ? "" : ", no output image");
        return NULL;
    }
    return image;
}

bool
isSentinel(const unsigned char* p)
{
    const unsigned int quietNaNBits = 0x7fc00000U; // hostDemo's sentinel (hostDemoClipInstance.cpp)
    return std::memcmp(p, &quietNaNBits, sizeof(float)) == 0;
}

bool
isZero(const unsigned char* p)
{
    const float zero = 0.f;
    return std::memcmp(p, &zero, sizeof(float)) == 0;
}

} // namespace

int
main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "Usage: %s <identifier> <major> [name=value...] [--scale s] [--window x1,y1,x2,y2]...\n", argv[0]);
        return 2;
    }
    const std::string pluginId = argv[1];
    const int pluginMajor = std::atoi(argv[2]);
    std::vector<std::string> assignments;
    std::vector<OfxRectI> windows;
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--scale" && i + 1 < argc) {
            char* end = NULL;
            const double s = std::strtod(argv[++i], &end);
            if (*end != '\0' || !(s > 0. && s <= 1.)) {
                std::fprintf(stderr, "Error: --scale s, with 0 < s <= 1\n");
                return 2;
            }
            renderScale.x = renderScale.y = s;
        } else if (arg == "--window" && i + 1 < argc) {
            std::vector<int> v;
            if (!parseInts(argv[++i], v) || v.size() != 4) {
                std::fprintf(stderr, "Error: --window x1,y1,x2,y2\n");
                return 2;
            }
            OfxRectI w;
            w.x1 = v[0];
            w.y1 = v[1];
            w.x2 = v[2];
            w.y2 = v[3];
            windows.push_back(w);
        } else {
            assignments.push_back(arg);
        }
    }
    std::string what = pluginId + " v" + argv[2];
    if (renderScale.x != 1.) {
        std::ostringstream ss;
        ss << what << " at scale " << renderScale.x;
        what = ss.str();
    }

    OFX::Host::PluginCache::useStdOFXPluginsLocation(false);
    OFX::Host::PluginCache::getPluginCache()->setCacheVersion("readerWindowV1");
    MyHost::Host myHost;
    OFX::Host::ImageEffect::PluginCache imageEffectPluginCache(&myHost);
    imageEffectPluginCache.registerInCache(*OFX::Host::PluginCache::getPluginCache());
    OFX::Host::PluginCache::getPluginCache()->scanPluginFiles();
    OFX::Host::ImageEffect::ImageEffectPlugin* plugin = imageEffectPluginCache.getPluginById(pluginId, pluginMajor);
    if (!plugin) {
        std::fprintf(stderr, "Error: %s not found in OFX_PLUGIN_PATH\n", what.c_str());
        return 1;
    }

    int failures = 0;
    {
        OFX::Host::auto_ptr<OFX::Host::ImageEffect::Instance> instance(plugin->createInstance(kOfxImageEffectContextGeneral, NULL));
        // send kOfxActionDestroyInstance while the instance is still whole, before it is deleted
        // (the guard is destroyed before the auto_ptr), as hostDemo does (openfx#190): the
        // HostSupport destructor only sends it as a safety net, once the overridden virtuals are gone
        struct DestroyInstanceGuard {
            OFX::Host::ImageEffect::Instance* instance;
            ~DestroyInstanceGuard() { if (instance) { instance->destroyInstanceAction(); } }
        } destroyInstanceGuard = { instance.get() };
        MyHost::MyEffectInstance* myInstance = dynamic_cast<MyHost::MyEffectInstance*>(instance.get());
        if (!myInstance) {
            std::fprintf(stderr, "Error: could not create an instance of %s\n", what.c_str());
            return 1;
        }
        myInstance->setDefaultDepth(kOfxBitDepthFloat); // float output: bit-exact comparisons
        OfxStatus stat = instance->createInstanceAction();
        if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
            std::fprintf(stderr, "Error: create instance action failed (status %d)\n", (int)stat);
            return 1;
        }
        if (!instance->getClipPreferences()) {
            std::fprintf(stderr, "Error: get clip preferences action failed\n");
            return 1;
        }
        for (size_t k = 0; k < assignments.size(); ++k) {
            if (!setParam(instance.get(), assignments[k])) {
                return 1;
            }
        }
        if (instance->areClipPrefsDirty() && !instance->getClipPreferences()) {
            std::fprintf(stderr, "Error: get clip preferences action failed\n");
            return 1;
        }
        MyHost::MyClipInstance* outputClip = dynamic_cast<MyHost::MyClipInstance*>(instance->getClip(kOfxImageEffectOutputClipName));
        if (!outputClip) {
            std::fprintf(stderr, "Error: no output clip\n");
            return 1;
        }
        if (outputClip->getPixelDepth() != kOfxBitDepthFloat) {
            std::fprintf(stderr, "Error: the output depth is %s, not float\n", outputClip->getPixelDepth().c_str());
            return 1;
        }
        instance->timeLineGotoTime(0.);
        OfxRectD rodD;
        rodD.x1 = rodD.y1 = rodD.x2 = rodD.y2 = 0.;
        stat = instance->getRegionOfDefinitionAction(0., renderScale,
#ifdef OFX_EXTENSIONS_NUKE
                                                     0,
#endif
                                                     rodD);
        if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
            std::fprintf(stderr, "Error: get region of definition action failed (status %d)\n", (int)stat);
            return 1;
        }
        const double par = instance->getProjectPixelAspectRatio();
        const OfxRectI rod = MyHost::canonicalToPixelEnclosing(rodD, renderScale, par);
        const int w = rod.x2 - rod.x1;
        const int h = rod.y2 - rod.y1;
        if (w < 4 || h < 4) {
            std::fprintf(stderr, "Error: the RoD (%d,%d,%d,%d) is too small for the test\n", rod.x1, rod.y1, rod.x2, rod.y2);
            return 1;
        }
        if (windows.empty()) {
            // the middle quarter, and windows at the edges of the image, none at the origin; then
            // windows partly or entirely outside of the RoD, which GenericReader clears and clips
            // to the RoD before decode() (#108)
            const int def[][4] = {
                { w / 4, h / 4, 3 * w / 4, 3 * h / 4 }, // the middle quarter
                { w / 3, h / 2 + 1, w, h },             // the top right part (odd offsets)
                { 1, 0, w / 5 + 1, h / 3 },             // along the bottom edge
                { 0, h / 2, w, h / 2 + 1 },             // one row
                { w - 1, 1, w, h - 1 },                 // one column, at the right edge
                { w / 2, h / 2, w / 2 + 1, h / 2 + 1 }, // one pixel
                { -3, -2, w / 4, h / 4 },               // across the bottom left corner
                { 3 * w / 4, h / 2, w + 5, h + 3 },     // across the top right corner
                { -2, h / 3, w + 2, h / 3 + 2 },        // two rows, wider than the RoD
                { -4, -3, w + 4, h + 3 },               // the RoD and a border around it
                { w + 1, 1, w + 4, h / 2 },             // right of the RoD
                { -6, -5, -1, -1 },                     // below and left of the RoD
            };
            for (size_t k = 0; k < sizeof(def) / sizeof(def[0]); ++k) {
                OfxRectI r;
                r.x1 = rod.x1 + def[k][0];
                r.y1 = rod.y1 + def[k][1];
                r.x2 = rod.x1 + def[k][2];
                r.y2 = rod.y1 + def[k][3];
                windows.push_back(r);
            }
        }

        stat = instance->beginRenderAction(0., 0., 1., false, renderScale, false, /*interactive=*/false,
#ifdef OFX_SUPPORTS_OPENGLRENDER
                                           /*openGLRender=*/false,
#  ifdef OFX_EXTENSIONS_NATRON
                                           /*contextData=*/NULL,
#  endif
#endif
                                           /*draftRender=*/false
#ifdef OFX_EXTENSIONS_NUKE
                                           , 0 /* view*/
#endif
                                           );
        if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
            std::fprintf(stderr, "Error: begin render action failed (status %d)\n", (int)stat);
            return 1;
        }

        // the reference: the whole RoD
        MyHost::MyImage* full = render(myInstance, outputClip, renderScale, rod, rod);
        std::vector<unsigned char> ref;
        int nComps = 0;
        size_t pixelBytes = 0;
        if (!full) {
            ++failures;
        } else {
            nComps = full->getNComponents();
            pixelBytes = (size_t)nComps * full->getBytesPerComponent();
            ref.assign(full->getPixelData(rod.x1, rod.y1), full->getPixelData(rod.x1, rod.y1) + pixelBytes * (size_t)w * (size_t)h);
            long unwritten = 0;
            for (size_t i = 0; i + sizeof(float) <= ref.size(); i += sizeof(float)) {
                unwritten += isSentinel(&ref[i]);
            }
            if (unwritten) {
                std::fprintf(stderr, "FAIL %s: the full render left %ld components unwritten\n", what.c_str(), unwritten);
                ++failures;
            }
        }

        for (size_t k = 0; full && k < windows.size(); ++k) {
            const OfxRectI& win = windows[k];
            MyHost::MyImage* img = render(myInstance, outputClip, renderScale, rod, win);
            if (!img) {
                std::printf("FAIL %s window (%d,%d,%d,%d): render failed\n", what.c_str(), win.x1, win.y1, win.x2, win.y2);
                ++failures;
                continue;
            }
            long differ = 0;
            long unwritten = 0;
            long outsideWritten = 0;
            long notBlack = 0;
            // the whole output image: the RoD and the window
            const OfxRectI all = unionRect(rod, win);
            for (int y = all.y1; y < all.y2; ++y) {
                for (int x = all.x1; x < all.x2; ++x) {
                    const unsigned char* p = img->getPixelData(x, y);
                    const bool inside = x >= win.x1 && x < win.x2 && y >= win.y1 && y < win.y2;
                    const bool inRoD = x >= rod.x1 && x < rod.x2 && y >= rod.y1 && y < rod.y2;
                    for (int c = 0; c < nComps; ++c) {
                        const unsigned char* pc = p + c * sizeof(float);
                        if (inside) {
                            if (isSentinel(pc)) {
                                ++unwritten;
                            } else if (!inRoD) {
                                notBlack += !isZero(pc);
                            } else if (std::memcmp(pc, &ref[((size_t)(y - rod.y1) * w + (x - rod.x1)) * pixelBytes + c * sizeof(float)], sizeof(float)) != 0) {
                                ++differ;
                            }
                        } else if (!isSentinel(pc)) {
                            ++outsideWritten;
                        }
                    }
                }
            }
            const bool ok = !differ && !unwritten && !outsideWritten && !notBlack;
            std::printf("%s %s window (%d,%d,%d,%d) of (%d,%d,%d,%d): %ld components differ from the full render, %ld unwritten, %ld written outside, %ld not black outside of the RoD\n",
                        ok ? "ok  " : "FAIL", what.c_str(), win.x1, win.y1, win.x2, win.y2, rod.x1, rod.y1, rod.x2, rod.y2,
                        differ, unwritten, outsideWritten, notBlack);
            if (!ok) {
                ++failures;
            }
        }
        outputClip->releaseOutputImage();
        instance->endRenderAction(0., 0., 1., false, renderScale, false, /*interactive=*/false,
#ifdef OFX_SUPPORTS_OPENGLRENDER
                                  /*openGLRender=*/false,
#  ifdef OFX_EXTENSIONS_NATRON
                                  /*contextData=*/NULL,
#  endif
#endif
                                  /*draftRender=*/false
#ifdef OFX_EXTENSIONS_NUKE
                                  , 0 /* view*/
#endif
                                  );
    }
    std::printf("%s: %s\n", what.c_str(), failures ? "FAILED" : "all windows OK");
    return failures ? 1 : 0;
}
