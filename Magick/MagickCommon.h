#ifndef MagickCommon_h
#define MagickCommon_h

#include <cassert>
#include <cstdlib>
#include <Magick++.h>
#ifdef IsNaN
#undef IsNaN
#endif

// Initialize ImageMagick and set its resource limits, once in the process, before the first
// render: the limits are process-wide and the plug-ins render concurrently, so setting them per
// instance or per render raced (#251). A thread limit raised during another render even
// overflows ImageMagick's per-thread arrays, sized before its parallel loops read the limit
// again. One thread, because the random-number operations (Texture's noise) give
// thread-count-dependent pixels; ImageMagick reads MAGICK_THREAD_LIMIT itself, so leave the
// limit to it when that is set. No disk cache: an image over the memory limit fails to render.
inline void magickInitialize()
{
    static const bool initialized = [] {
        Magick::InitializeMagick(NULL);
        Magick::ResourceLimits::disk(0);
        if (std::getenv("MAGICK_THREAD_LIMIT") == NULL) {
            Magick::ResourceLimits::thread(1);
        }
        return true;
    }();
    (void)initialized;
}

#endif // MagickCommon_h
