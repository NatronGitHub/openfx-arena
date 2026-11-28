/*
 * mt_unload: render a plug-in with several threads, then unload its binary, and exit.
 *
 *   mt_unload [--threads N] [--frames N] [--wait ms] [--min-threads N] <identifier> <major> <filter|general>
 *             [name=value...]
 *
 * Checks that the plug-ins survive a host that unloads the binary after a multi-threaded render,
 * as openfx's hostDemo does at exit (PluginCache::clearPluginCache(), openfx #174). openfx-gmic
 * crashed there (SIGSEGV, or 0xC0000005 on Windows) after a successful render: worker threads
 * started by a runtime that the binary links (MSVC's OpenMP, FFTW's threads) were still running,
 * or were torn down by a static destructor of the unloaded binary. Arena's plug-ins start such
 * threads too: ImageMagick's OpenMP (libgomp, libomp), glib's thread pools (pango, librsvg,
 * poppler).
 *
 * hostDemo can't do this: its multi-thread suite (HostSupport without OFX_SUPPORTS_MULTITHREAD)
 * runs everything on the calling thread and says there is one CPU. This
 * host is hostDemo's host classes with a real OfxMultiThreadSuiteV1 (N CPUs, N threads per
 * multiThread call), so that
 *   - the plug-ins' OFX::MultiThread::Processor code runs on N threads,
 *   - the Magick plug-ins run ImageMagick on OpenMP threads (with MAGICK_THREAD_LIMIT set, see
 *     mt_unload.cmake; without it ImageMagick runs on one thread), which stay in the OpenMP runtime's
 *     pool after the render.
 * It scans the plug-ins in hostDemo's default mode (the binary is unloaded after the scan and
 * loaded again at the first use, unless OFX_HOSTDEMO_KEEP_LOADED=1), creates the instance,
 * renders frames 0..N-1 (the RoD, clipped to the project), destroys the instance, unloads every
 * binary (PluginCache::clearPluginCache(), as hostDemo's exit), waits a little (--wait, default
 * 500 ms: an idle OpenMP thread spins, or wakes up, in the runtime's code, which is gone if the
 * runtime was unloaded with the binary), prints which images are still loaded, and returns from
 * main(), so that the static destructors and atexit handlers run with the binary unloaded.
 * --min-threads N fails the test if the process has fewer than N threads after the render (default:
 * 1, no check): with 2, a plug-in that should leave OpenMP pool threads behind must do so, so that an
 * ImageMagick without OpenMP, or a parameter change, doesn't silently make the test a no-op.
 * The default input is hostDemo's (PAL SD, byte). Exit status: 0 if everything succeeds; a crash
 * at any point, including at exit, is a failure of the test.
 */

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach/mach.h>
#else
#include <dirent.h>
#include <link.h>
#endif

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxMultiThread.h"
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

// --- a multi-thread suite with real threads ---------------------------------------------------

unsigned int gNumThreads = 4;
// the index of the current thread in its multiThread call, and whether it is a spawned thread
thread_local unsigned int tThreadIndex = 0;
thread_local bool tSpawned = false;
std::atomic<bool> gInMultiThread(false);

OfxStatus
mtMultiThread(OfxThreadFunctionV1 func, unsigned int nThreads, void* customArg)
{
    if (!func) {
        return kOfxStatFailed;
    }
    // "This function cannot be called recursively" (ofxMultiThread.h)
    if (tSpawned || gInMultiThread.exchange(true)) {
        return kOfxStatErrExists;
    }
    if (nThreads == 0) {
        nThreads = 1;
    }
    std::vector<std::thread> threads;
    threads.reserve(nThreads);
    for (unsigned int i = 0; i < nThreads; ++i) {
        threads.push_back(std::thread([=]() {
            tThreadIndex = i;
            tSpawned = true;
            func(i, nThreads, customArg);
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    gInMultiThread = false;
    return kOfxStatOK;
}

OfxStatus
mtNumCPUs(unsigned int* nCPUs)
{
    if (!nCPUs) {
        return kOfxStatFailed;
    }
    *nCPUs = gNumThreads;
    return kOfxStatOK;
}

OfxStatus
mtIndex(unsigned int* threadIndex)
{
    if (!threadIndex) {
        return kOfxStatFailed;
    }
    *threadIndex = tSpawned ? tThreadIndex : 0;
    return kOfxStatOK;
}

int
mtIsSpawnedThread(void)
{
    return tSpawned ? 1 : 0;
}

OfxStatus
mtMutexCreate(OfxMutexHandle* mutex, int lockCount)
{
    if (!mutex) {
        return kOfxStatFailed;
    }
    std::recursive_mutex* m = new std::recursive_mutex;
    for (int i = 0; i < lockCount; ++i) {
        m->lock();
    }
    *mutex = (OfxMutexHandle)m;
    return kOfxStatOK;
}

OfxStatus
mtMutexDestroy(const OfxMutexHandle mutex)
{
    if (!mutex) {
        return kOfxStatErrBadHandle;
    }
    delete (std::recursive_mutex*)mutex;
    return kOfxStatOK;
}

OfxStatus
mtMutexLock(const OfxMutexHandle mutex)
{
    if (!mutex) {
        return kOfxStatErrBadHandle;
    }
    ((std::recursive_mutex*)mutex)->lock();
    return kOfxStatOK;
}

OfxStatus
mtMutexUnLock(const OfxMutexHandle mutex)
{
    if (!mutex) {
        return kOfxStatErrBadHandle;
    }
    ((std::recursive_mutex*)mutex)->unlock();
    return kOfxStatOK;
}

OfxStatus
mtMutexTryLock(const OfxMutexHandle mutex)
{
    if (!mutex) {
        return kOfxStatErrBadHandle;
    }
    return ((std::recursive_mutex*)mutex)->try_lock() ? kOfxStatOK : kOfxStatFailed;
}

const OfxMultiThreadSuiteV1 gMultiThreadSuite = {
    mtMultiThread, mtNumCPUs, mtIndex, mtIsSpawnedThread,
    mtMutexCreate, mtMutexDestroy, mtMutexLock, mtMutexUnLock, mtMutexTryLock
};

// hostDemo's host, with the multi-thread suite above
class MtHost : public MyHost::Host
{
public:
    virtual const void* fetchSuite(const char* suiteName, int suiteVersion)
    {
        if (std::strcmp(suiteName, kOfxMultiThreadSuite) == 0) {
            return suiteVersion == 1 ? &gMultiThreadSuite : NULL;
        }
        return MyHost::Host::fetchSuite(suiteName, suiteVersion);
    }
};

// --- the actions ------------------------------------------------------------------------------

bool
parseInt(const std::string& s, int* v)
{
    char* end = NULL;
    const long l = std::strtol(s.c_str(), &end, 10);
    if (s.empty() || *end != '\0') {
        return false;
    }
    *v = (int)l;
    return true;
}

// set a parameter from "name=value" (string, integer, choice index, double, boolean), then send
// the instance changed actions
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
    int iv = 0;
    if (OFX::Host::Param::StringInstance* s = dynamic_cast<OFX::Host::Param::StringInstance*>(p)) {
        st = s->set(value.c_str());
    } else if (OFX::Host::Param::IntegerInstance* i = dynamic_cast<OFX::Host::Param::IntegerInstance*>(p)) {
        st = parseInt(value, &iv) ? i->set(iv) : kOfxStatErrValue;
    } else if (OFX::Host::Param::ChoiceInstance* c = dynamic_cast<OFX::Host::Param::ChoiceInstance*>(p)) {
        st = parseInt(value, &iv) ? c->set(iv) : kOfxStatErrValue;
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

// render frames 0..nFrames-1, each with its RoD clipped to the project as the render window
bool
renderFrames(MyHost::MyEffectInstance* instance, int nFrames)
{
    OfxPointD scale;
    scale.x = scale.y = 1.;
    const double par = instance->getProjectPixelAspectRatio();
    double pw = 0., ph = 0., px = 0., py = 0.;
    instance->getProjectSize(pw, ph);
    instance->getProjectOffset(px, py);
    OfxRectD project;
    project.x1 = px;
    project.y1 = py;
    project.x2 = px + pw;
    project.y2 = py + ph;
    MyHost::MyClipInstance* outputClip = dynamic_cast<MyHost::MyClipInstance*>(instance->getClip(kOfxImageEffectOutputClipName));
    if (!outputClip) {
        std::fprintf(stderr, "Error: no output clip\n");
        return false;
    }
    OfxStatus stat = instance->beginRenderAction(0., nFrames - 1, 1., false, scale, /*sequential=*/true, /*interactive=*/false,
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
        return false;
    }
    bool ok = true;
    for (int t = 0; t < nFrames && ok; ++t) {
        instance->timeLineGotoTime(t);
        OfxRectD rodD;
        rodD.x1 = rodD.y1 = rodD.x2 = rodD.y2 = 0.;
        stat = instance->getRegionOfDefinitionAction(t, scale,
#ifdef OFX_EXTENSIONS_NUKE
                                                     0,
#endif
                                                     rodD);
        if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
            std::fprintf(stderr, "Error: get region of definition action failed at t=%d (status %d)\n", t, (int)stat);
            ok = false;
            break;
        }
        OfxRectD w;
        w.x1 = rodD.x1 > project.x1 ? rodD.x1 : project.x1;
        w.y1 = rodD.y1 > project.y1 ? rodD.y1 : project.y1;
        w.x2 = rodD.x2 < project.x2 ? rodD.x2 : project.x2;
        w.y2 = rodD.y2 < project.y2 ? rodD.y2 : project.y2;
        const OfxRectI window = MyHost::canonicalToPixelEnclosing(w, scale, par);
        if (window.x2 <= window.x1 || window.y2 <= window.y1) {
            std::fprintf(stderr, "Error: empty render window at t=%d\n", t);
            ok = false;
            break;
        }
        std::map<OFX::Host::ImageEffect::ClipInstance*, OfxRectD> rois;
        instance->setRenderState(scale, window, rois);
        outputClip->releaseOutputImage();
        const int imageErrors = MyHost::gImageErrors;
#ifdef OFX_EXTENSIONS_NUKE
        std::list<std::string> planes;
        planes.push_back(kFnOfxImagePlaneColour);
#endif
        stat = instance->renderAction(t, kOfxImageFieldNone, window, scale,
                                      /*sequentialRender=*/true, /*interactiveRender=*/false,
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
        if (stat != kOfxStatOK || !outputClip->getOutputImage() || MyHost::gImageErrors != imageErrors) {
            std::fprintf(stderr, "Error: render at t=%d failed (status %d%s)\n", t, (int)stat,
                         outputClip->getOutputImage() ? "" : ", no output image");
            ok = false;
        }
        outputClip->releaseOutputImage();
    }
    instance->endRenderAction(0., nFrames - 1, 1., false, scale, /*sequential=*/true, /*interactive=*/false,
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
    return ok;
}

// --- what is still loaded after the unload (diagnostics only) ---------------------------------

// true for the images worth reporting: the plug-in binary, the OpenMP runtimes, ImageMagick,
// glib, and the libraries whose threads the plug-ins may start
bool
interesting(const std::string& path)
{
    static const char* const kNames[] = {
        ".ofx", "libomp", "gomp", "iomp5", "vcomp", "Magick", "glib", "gio", "gobject", "pango", "rsvg", "poppler", "cairo",
        "fftw", "sox", "fontconfig", "harfbuzz",
    };
    std::string base = path;
    const std::string::size_type slash = base.find_last_of("/\\");
    if (slash != std::string::npos) {
        base = base.substr(slash + 1);
    }
    for (size_t k = 0; k < sizeof(kNames) / sizeof(kNames[0]); ++k) {
        if (base.find(kNames[k]) != std::string::npos) {
            return true;
        }
    }
    return false;
}

#if !defined(_WIN32) && !defined(__APPLE__)
int
addImage(struct dl_phdr_info* info, size_t, void* data)
{
    if (info->dlpi_name && info->dlpi_name[0]) {
        static_cast<std::vector<std::string>*>(data)->push_back(info->dlpi_name);
    }
    return 0;
}
#endif

std::vector<std::string>
loadedImages()
{
    std::vector<std::string> images;
#if defined(_WIN32)
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snap != INVALID_HANDLE_VALUE) {
        // the W functions, whether UNICODE is defined or not
        MODULEENTRY32W me;
        me.dwSize = sizeof(me);
        for (BOOL more = Module32FirstW(snap, &me); more; more = Module32NextW(snap, &me)) {
            char path[4 * MAX_PATH];
            if (WideCharToMultiByte(CP_UTF8, 0, me.szExePath, -1, path, (int)sizeof(path), NULL, NULL) > 0) {
                images.push_back(path);
            }
        }
        CloseHandle(snap);
    }
#elif defined(__APPLE__)
    const uint32_t n = _dyld_image_count();
    for (uint32_t i = 0; i < n; ++i) {
        const char* name = _dyld_get_image_name(i);
        if (name) {
            images.push_back(name);
        }
    }
#else
    dl_iterate_phdr(addImage, &images);
#endif
    std::vector<std::string> kept;
    for (size_t k = 0; k < images.size(); ++k) {
        if (interesting(images[k])) {
            kept.push_back(images[k]);
        }
    }
    return kept;
}

// the number of threads of the process (0 if unknown): the threads that the render left behind
int
threadCount()
{
    int n = 0;
#if defined(_WIN32)
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        for (BOOL more = Thread32First(snap, &te); more; more = Thread32Next(snap, &te)) {
            if (te.th32OwnerProcessID == GetCurrentProcessId()) {
                ++n;
            }
        }
        CloseHandle(snap);
    }
#elif defined(__APPLE__)
    thread_act_array_t threads;
    mach_msg_type_number_t count = 0;
    if (task_threads(mach_task_self(), &threads, &count) == KERN_SUCCESS) {
        n = (int)count;
        for (mach_msg_type_number_t k = 0; k < count; ++k) {
            mach_port_deallocate(mach_task_self(), threads[k]);
        }
        vm_deallocate(mach_task_self(), (vm_address_t)threads, count * sizeof(thread_act_t));
    }
#else
    if (DIR* d = opendir("/proc/self/task")) {
        while (struct dirent* e = readdir(d)) {
            if (e->d_name[0] != '.') {
                ++n;
            }
        }
        closedir(d);
    }
#endif
    return n;
}

} // namespace

int
main(int argc, char** argv)
{
    int nFrames = 2;
    int waitMs = 500;
    int minThreads = 1;
    int i = 1;
    for (; i < argc && std::strncmp(argv[i], "--", 2) == 0; ++i) {
        const std::string arg = argv[i];
        int v = 0;
        if (arg == "--threads" && i + 1 < argc && parseInt(argv[i + 1], &v) && v >= 1) {
            gNumThreads = (unsigned int)v;
        } else if (arg == "--frames" && i + 1 < argc && parseInt(argv[i + 1], &v) && v >= 1) {
            nFrames = v;
        } else if (arg == "--wait" && i + 1 < argc && parseInt(argv[i + 1], &v) && v >= 0) {
            waitMs = v;
        } else if (arg == "--min-threads" && i + 1 < argc && parseInt(argv[i + 1], &v) && v >= 1) {
            minThreads = v;
        } else {
            argc = 0; // usage
            break;
        }
        ++i;
    }
    if (argc - i < 3) {
        std::fprintf(stderr, "Usage: %s [--threads N] [--frames N] [--wait ms] [--min-threads N] <identifier> <major> <filter|general> [name=value...]\n", argv[0]);
        return 2;
    }
    const std::string pluginId = argv[i];
    const int pluginMajor = std::atoi(argv[i + 1]);
    const std::string ctx = argv[i + 2];
    const std::string context = ctx == "general" ? kOfxImageEffectContextGeneral : ctx == "filter" ? kOfxImageEffectContextFilter : "";
    if (context.empty()) {
        std::fprintf(stderr, "Error: the context is filter or general, not %s\n", ctx.c_str());
        return 2;
    }
    std::vector<std::string> assignments(argv + i + 3, argv + argc);
    const std::string what = pluginId + " v" + argv[i + 1];

    OFX::Host::PluginCache::useStdOFXPluginsLocation(false);
    OFX::Host::PluginCache::getPluginCache()->setCacheVersion("mtUnloadV1");
    int ret = 0;
    int threadsAfterRender = 0;
    {
        // the host and the image effect cache outlive the instance, as in hostDemo
        MtHost myHost;
        OFX::Host::ImageEffect::PluginCache imageEffectPluginCache(&myHost);
        imageEffectPluginCache.registerInCache(*OFX::Host::PluginCache::getPluginCache());
        OFX::Host::PluginCache::getPluginCache()->scanPluginFiles();
        OFX::Host::ImageEffect::ImageEffectPlugin* plugin = imageEffectPluginCache.getPluginById(pluginId, pluginMajor);
        if (!plugin) {
            std::fprintf(stderr, "Error: %s not found in OFX_PLUGIN_PATH\n", what.c_str());
            return 1;
        }
        {
            OFX::Host::auto_ptr<OFX::Host::ImageEffect::Instance> instance(plugin->createInstance(context, NULL));
            // send kOfxActionDestroyInstance while the instance is still whole, as hostDemo does
            // (openfx#190), before the auto_ptr deletes it
            struct DestroyInstanceGuard {
                OFX::Host::ImageEffect::Instance* instance;
                ~DestroyInstanceGuard() { if (instance) { instance->destroyInstanceAction(); } }
            } destroyInstanceGuard = { instance.get() };
            MyHost::MyEffectInstance* myInstance = dynamic_cast<MyHost::MyEffectInstance*>(instance.get());
            if (!myInstance) {
                std::fprintf(stderr, "Error: could not create an instance of %s\n", what.c_str());
                return 1;
            }
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
            if (!renderFrames(myInstance, nFrames)) {
                ret = 1;
            }
        }
        threadsAfterRender = threadCount();
        if (threadsAfterRender < minThreads) {
            std::fprintf(stderr, "Error: %d thread(s) in the process after the render, expected at least %d: "
                         "the render didn't leave the threads that this test is about (no OpenMP?)\n",
                         threadsAfterRender, minThreads);
            ret = 1;
        }
        // unload every binary, as hostDemo's exit does (the plug-ins get kOfxActionUnload)
        OFX::Host::PluginCache::clearPluginCache();
    }
    // give the threads that the plug-ins left behind the time to run into an unloaded runtime
    std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
    const std::vector<std::string> images = loadedImages();
    std::printf("%s: rendered %d frame(s) with %u threads%s; %d threads in the process after the render, %d after the unload; "
                "%zu of the plug-in's images still loaded%s\n",
                what.c_str(), nFrames, gNumThreads, ret ? " (FAILED)" : "", threadsAfterRender, threadCount(),
                images.size(), images.empty() ? "" : ":");
    for (size_t k = 0; k < images.size(); ++k) {
        std::printf("  %s\n", images[k].c_str());
    }
    std::fflush(stdout);
    return ret; // the static destructors and atexit handlers run now, without the binary
}
