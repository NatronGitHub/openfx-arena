/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * openfx-arena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
*/

/*
 * Helpers to exchange pixels between an OFX image buffer and a tightly packed
 * float buffer (as used by ImageMagick, cairo-based renderers, etc.).
 *
 * The OFX spec (ofxImageEffect.h) says about kOfxImagePropRowBytes:
 *   "For various alignment reasons, a row of pixels may need to be padded at
 *    the end with several bytes before the next row starts in memory. [...]
 *    This will be at least sizeof(PIXEL) * (bounds.x2-bounds.x1). [...]
 *    Note that row bytes can be negative, which allows hosts with a native top
 *    down row order to pass image into OFX without having to repack pixels."
 * and about kOfxImagePropBounds:
 *   "The bounds, in PixelCoordinates, are of the addressable pixels in an
 *    image's data pointer."
 *
 * So the pixel (x,y) of an image is at
 *   data + (y - bounds.y1) * rowBytes + (x - bounds.x1) * pixelBytes
 * and nothing guarantees that bounds equals the RoD or the render window, or
 * that rows are contiguous.
 *
 * In the packed buffers handled here, row r holds the pixels of
 * y = window.y1 + r (bottom-up, like OFX), and column c holds x = window.x1 + c.
 *
 * When the image buffer already is exactly the packed window (positive,
 * unpadded rowBytes and bounds == window), PackedSrc/PackedDst use the image
 * memory directly, so the result is bit-identical to accessing it in place.
 */

#ifndef Arena_PackedImage_h
#define Arena_PackedImage_h

#include <cstddef>
#include <cstring>
#include <vector>

#include "ofxCore.h"

namespace PackedImage {

/// Address of pixel (x,y) of an OFX image buffer. rowBytes may be negative.
inline const float*
pixelAddress(const void* data, const OfxRectI& bounds, int rowBytes, int nComps, int x, int y)
{
    return reinterpret_cast<const float*>(static_cast<const char*>(data) + (std::ptrdiff_t)(y - bounds.y1) * rowBytes)
           + (std::ptrdiff_t)(x - bounds.x1) * nComps;
}

inline float*
pixelAddress(void* data, const OfxRectI& bounds, int rowBytes, int nComps, int x, int y)
{
    return reinterpret_cast<float*>(static_cast<char*>(data) + (std::ptrdiff_t)(y - bounds.y1) * rowBytes)
           + (std::ptrdiff_t)(x - bounds.x1) * nComps;
}

/// True if the image buffer is exactly a packed buffer for window.
inline bool
isPacked(const OfxRectI& bounds, int rowBytes, int nComps, const OfxRectI& window)
{
    return bounds.x1 == window.x1 && bounds.x2 == window.x2 && bounds.y1 == window.y1 && bounds.y2 == window.y2 &&
           (std::ptrdiff_t)rowBytes == (std::ptrdiff_t)(window.x2 - window.x1) * nComps * (std::ptrdiff_t)sizeof(float);
}

/// Copy window from an OFX image buffer to a packed buffer.
/// Pixels of window that are outside bounds are set to 0 (transparent black).
inline void
imageToPacked(const void* data, const OfxRectI& bounds, int rowBytes, int nComps,
              const OfxRectI& window, float* packed)
{
    const int w = window.x2 - window.x1;
    const int h = window.y2 - window.y1;
    if (w <= 0 || h <= 0) {
        return;
    }
    const std::size_t packedRow = (std::size_t)w * nComps;
    // x range of window covered by bounds
    const int x1 = window.x1 > bounds.x1 ? window.x1 : bounds.x1;
    const int x2 = window.x2 < bounds.x2 ? window.x2 : bounds.x2;
    for (int r = 0; r < h; ++r) {
        const int y = window.y1 + r;
        float* dst = packed + (std::size_t)r * packedRow;
        if (!data || y < bounds.y1 || y >= bounds.y2 || x1 >= x2) {
            std::memset(dst, 0, packedRow * sizeof(float));
            continue;
        }
        if (window.x1 < x1) {
            std::memset(dst, 0, (std::size_t)(x1 - window.x1) * nComps * sizeof(float));
        }
        std::memcpy(dst + (std::size_t)(x1 - window.x1) * nComps,
                    pixelAddress(data, bounds, rowBytes, nComps, x1, y),
                    (std::size_t)(x2 - x1) * nComps * sizeof(float));
        if (x2 < window.x2) {
            std::memset(dst + (std::size_t)(x2 - window.x1) * nComps, 0, (std::size_t)(window.x2 - x2) * nComps * sizeof(float));
        }
    }
}

/// Copy a packed buffer for window to an OFX image buffer.
/// Only the part of window inside bounds is written.
inline void
packedToImage(const float* packed, const OfxRectI& window, int nComps,
              void* data, const OfxRectI& bounds, int rowBytes)
{
    const int w = window.x2 - window.x1;
    const int h = window.y2 - window.y1;
    if (!data || w <= 0 || h <= 0) {
        return;
    }
    const std::size_t packedRow = (std::size_t)w * nComps;
    const int x1 = window.x1 > bounds.x1 ? window.x1 : bounds.x1;
    const int x2 = window.x2 < bounds.x2 ? window.x2 : bounds.x2;
    if (x1 >= x2) {
        return;
    }
    for (int r = 0; r < h; ++r) {
        const int y = window.y1 + r;
        if (y < bounds.y1 || y >= bounds.y2) {
            continue;
        }
        std::memcpy(pixelAddress(data, bounds, rowBytes, nComps, x1, y),
                    packed + (std::size_t)r * packedRow + (std::size_t)(x1 - window.x1) * nComps,
                    (std::size_t)(x2 - x1) * nComps * sizeof(float));
    }
}

/// Divide the colour of packed RGBA pixels by their alpha.
/// Pixels with alpha <= 0 are left unchanged, as in openfx-misc's Unpremult.
inline void
unpremultiplyRGBA(float* packed, std::size_t nPixels)
{
    for (std::size_t i = 0; i < nPixels; ++i, packed += 4) {
        const float a = packed[3];
        if (a > 0.f && a != 1.f) {
            packed[0] /= a;
            packed[1] /= a;
            packed[2] /= a;
        }
    }
}

/// Read-only packed view of window of an OFX image buffer.
/// No copy is made if the buffer already is packed and covers exactly window,
/// unless unpremult is set: then the packed RGBA pixels are a copy with colour
/// divided by alpha (for libraries such as ImageMagick that expect straight colour).
class PackedSrc
{
public:
    PackedSrc(const void* data, const OfxRectI& bounds, int rowBytes, int nComps, const OfxRectI& window,
              bool unpremult = false)
        : _buf()
        , _ptr(NULL)
    {
        init(data, bounds, rowBytes, nComps, window, unpremult);
    }

    /// IMG is OFX::Image (template so that this header does not depend on the OFX Support library)
    template <class IMG>
    PackedSrc(const IMG* img, const OfxRectI& window, bool unpremult = false)
        : _buf()
        , _ptr(NULL)
    {
        init(img->getPixelData(), img->getBounds(), img->getRowBytes(), img->getPixelComponentCount(), window, unpremult);
    }

    const float* data() const { return _ptr; }

private:
    void init(const void* data, const OfxRectI& bounds, int rowBytes, int nComps, const OfxRectI& window, bool unpremult)
    {
        unpremult = unpremult && nComps == 4;
        if (data && !unpremult && isPacked(bounds, rowBytes, nComps, window)) {
            _ptr = static_cast<const float*>(data);
        } else {
            const int w = window.x2 - window.x1;
            const int h = window.y2 - window.y1;
            _buf.resize((std::size_t)(w > 0 ? w : 0) * (std::size_t)(h > 0 ? h : 0) * nComps);
            if (!_buf.empty()) {
                imageToPacked(data, bounds, rowBytes, nComps, window, &_buf[0]);
                if (unpremult) {
                    unpremultiplyRGBA(&_buf[0], _buf.size() / 4);
                }
                _ptr = &_buf[0];
            }
        }
    }

    PackedSrc(const PackedSrc&); // non-copyable: _ptr may point into _buf
    PackedSrc& operator=(const PackedSrc&);

    std::vector<float> _buf;
    const float* _ptr;
};

/// Packed buffer to be written to window of an OFX image buffer.
/// No copy is made if the buffer already is packed and covers exactly window:
/// data() then points into the image. Call commit() once the packed buffer is filled.
class PackedDst
{
public:
    PackedDst(void* data, const OfxRectI& bounds, int rowBytes, int nComps, const OfxRectI& window)
        : _buf()
        , _ptr(NULL)
        , _data(data)
        , _bounds(bounds)
        , _rowBytes(rowBytes)
        , _nComps(nComps)
        , _window(window)
    {
        init();
    }

    /// IMG is OFX::Image (template so that this header does not depend on the OFX Support library)
    template <class IMG>
    PackedDst(IMG* img, const OfxRectI& window)
        : _buf()
        , _ptr(NULL)
        , _data(img->getPixelData())
        , _bounds(img->getBounds())
        , _rowBytes(img->getRowBytes())
        , _nComps(img->getPixelComponentCount())
        , _window(window)
    {
        init();
    }

    float* data() { return _ptr; }

    /// copy the packed buffer to the image (no-op if data() points into the image)
    void commit()
    {
        if (!_buf.empty()) {
            packedToImage(&_buf[0], _window, _nComps, _data, _bounds, _rowBytes);
        }
    }

private:
    void init()
    {
        if (_data && isPacked(_bounds, _rowBytes, _nComps, _window)) {
            _ptr = static_cast<float*>(_data);
        } else {
            const int w = _window.x2 - _window.x1;
            const int h = _window.y2 - _window.y1;
            _buf.resize((std::size_t)(w > 0 ? w : 0) * (std::size_t)(h > 0 ? h : 0) * _nComps);
            if (!_buf.empty()) {
                _ptr = &_buf[0];
            }
        }
    }

    PackedDst(const PackedDst&); // non-copyable: _ptr may point into _buf
    PackedDst& operator=(const PackedDst&);

    std::vector<float> _buf;
    float* _ptr;
    void* _data;
    OfxRectI _bounds;
    int _rowBytes;
    int _nComps;
    OfxRectI _window;
};

} // namespace PackedImage

#endif // Arena_PackedImage_h
