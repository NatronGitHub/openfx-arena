#include <cassert>
#include <cstddef>
#include <vector>
#include <Magick++.h>
#ifdef IsNaN
#undef IsNaN
#endif

#ifndef MagickCommon_readRGBA
#define MagickCommon_readRGBA
// Load an OFX RGBA float buffer into a Magick::Image.
// ImageMagick stores straight (unassociated) colour: its Over composite
// computes Sc*Sa, so the "Over opaque black, then CopyAlpha" step that the
// Magick plugins use to write their output premultiplies the result. If the
// OFX source is premultiplied, it must therefore be unpremultiplied here, or
// the output would be premultiplied twice (C*a^2, darkening soft edges).
// Pixels with alpha <= 0 are left unchanged, as in openfx-misc's Unpremult.
inline void
magickReadRGBA(Magick::Image &image,
               int width,
               int height,
               const float *pixels,
               bool unpremult)
{
    const std::size_t n = (std::size_t)(width > 0 ? width : 0) * (std::size_t)(height > 0 ? height : 0) * 4;
    if (!unpremult || n == 0) {
        image.read(width, height, "RGBA", Magick::FloatPixel, pixels);
        return;
    }
    std::vector<float> buf(pixels, pixels + n);
    for (std::size_t i = 0; i < n; i += 4) {
        const float a = buf[i + 3];
        if (a > 0.f && a != 1.f) {
            buf[i] /= a;
            buf[i + 1] /= a;
            buf[i + 2] /= a;
        }
    }
    image.read(width, height, "RGBA", Magick::FloatPixel, &buf[0]);
}
#endif
