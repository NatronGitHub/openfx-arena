#ifndef MagickCommon_h
#define MagickCommon_h

#include <cassert>
#include <cstdlib>
#include <mutex>
#include <Magick++.h>
#ifdef IsNaN
#undef IsNaN
#endif
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"

// Initialize ImageMagick and set its resource limits, once in the process, before the first
// render: the limits are process-wide and the plug-ins render concurrently, so setting them per
// instance or per render raced (#251). A thread limit raised during another render even
// overflows ImageMagick's per-thread arrays, sized before its parallel loops read the limit
// again. One thread, because the random-number operations (Texture's noise) give
// thread-count-dependent pixels. No disk cache: an image over the memory limit fails to render.
// ImageMagick reads MAGICK_THREAD_LIMIT and MAGICK_DISK_LIMIT itself: leave each limit to it
// when its variable is set.
inline void magickInitialize()
{
    static const bool initialized = [] {
        Magick::InitializeMagick(nullptr);
        if (std::getenv("MAGICK_DISK_LIMIT") == nullptr) {
            Magick::ResourceLimits::disk(0);
        }
        if (std::getenv("MAGICK_THREAD_LIMIT") == nullptr) {
            Magick::ResourceLimits::thread(1);
        }
        return true;
    }();
    (void)initialized;
}

// ImageMagick's thread limit during a render, or during a reader's ImageMagick call (#265).
// ImageMagick sizes its per-thread arrays from the limit, then its parallel loops and the frees
// read it again: raising it during another call overflows them, and lowering it leaks their extra
// entries (whole images, for distort()'s resample filters). So the limit only changes when no other
// call in the bundle is active: to the host's thread count for an interactive render, to one
// thread otherwise, and back to one when the last call ends; a call that overlaps others keeps the
// current limit. The default constructor (readers, Texture) never raises it. With
// MAGICK_THREAD_LIMIT set, the limit is the user's: nothing changes.
class MagickThreads
{
public:
    MagickThreads()
        : MagickThreads(false)
    {
    }

    explicit MagickThreads(const OFX::RenderArguments& args)
        : MagickThreads(args.interactiveRenderStatus && !args.sequentialRenderStatus)
    {
    }

    ~MagickThreads()
    {
        if (!_counted) {
            return;
        }
        State& state = getState();
        std::lock_guard<std::mutex> lock(state.mutex);
        if (--state.active == 0) {
            Magick::ResourceLimits::thread(1);
        }
    }

    MagickThreads(const MagickThreads&) = delete;
    MagickThreads& operator=(const MagickThreads&) = delete;

private:
    explicit MagickThreads(bool severalThreads)
        : _counted(std::getenv("MAGICK_THREAD_LIMIT") == nullptr)
    {
        if (!_counted) {
            return;
        }
        // ask the host before locking: no host call under the lock
        const unsigned threads = severalThreads ? OFX::MultiThread::getNumCPUs() : 1;
        State& state = getState();
        std::lock_guard<std::mutex> lock(state.mutex);
        if (state.active == 0) {
            // ImageMagick caps it at omp_get_max_threads() and its policy's resource:thread
            Magick::ResourceLimits::thread(threads);
        }
        ++state.active;
    }

    struct State
    {
        std::mutex mutex;
        int active = 0; // the renders and reader calls using ImageMagick
    };

    static State& getState()
    {
        static State state;
        return state;
    }

    const bool _counted;
};

#endif // MagickCommon_h
