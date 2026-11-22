/*
 * openfx-arena <https://github.com/NatronGitHub/openfx-arena>,
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-arena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
*/

// Owning handles for the C objects of the readers (#132, #185), so that they are released on every
// path, including the throws of the OFX Support library. The deleters accept NULL. Also the size
// conversion of the vector readers.

#ifndef Arena_ReadHandles_h
#define Arena_ReadHandles_h

#include <cstdlib>
#include <memory>

#include <cairo.h>
#include <glib-object.h>
#include <zip.h>

namespace Arena {

// malloc'ed memory, e.g. lodepng's output (ReadKrita, OpenRaster)
struct FreeDeleter { void operator()(void* p) const { std::free(p); } };
template<typename T> using MallocPtr = std::unique_ptr<T, FreeDeleter>;

// ReadSVG, ReadCDR
struct CairoDeleter
{
    void operator()(cairo_t* p) const { if (p) { cairo_destroy(p); } }
    void operator()(cairo_surface_t* p) const { if (p) { cairo_surface_destroy(p); } }
};
using CairoPtr = std::unique_ptr<cairo_t, CairoDeleter>;
using CairoSurfacePtr = std::unique_ptr<cairo_surface_t, CairoDeleter>;

// a GObject with a reference that we own, e.g. GObjectPtr<RsvgHandle>; the GError that a failed
// call sets (ReadSVG, ReadCDR)
struct GObjectDeleter { void operator()(gpointer p) const { if (p) { g_object_unref(p); } } };
template<typename T> using GObjectPtr = std::unique_ptr<T, GObjectDeleter>;
struct GErrorDeleter { void operator()(GError* p) const { if (p) { g_error_free(p); } } };
using GErrorPtr = std::unique_ptr<GError, GErrorDeleter>;

// a zip archive opened ZIP_RDONLY (discarded: there is nothing to write back), and an entry of it
// (ReadKrita, OpenRaster)
struct ZipDeleter
{
    void operator()(zip_t* p) const { if (p) { zip_discard(p); } }
    void operator()(zip_file_t* p) const { if (p) { zip_fclose(p); } }
};
using ZipPtr = std::unique_ptr<zip_t, ZipDeleter>;
using ZipFilePtr = std::unique_ptr<zip_file_t, ZipDeleter>;

// A size in pixels computed in double from the file (ReadSVG, ReadPDF, ReadCDR), as an int,
// truncated; 0, which the readers report as an error, if it is NaN or outside [0, 32767] (cairo's
// limit): the conversion of a double outside of the int range is undefined (#158).
inline int
toPixels(double v)
{
    return (v >= 0. && v <= 32767.) ? static_cast<int>(v) : 0;
}

} // namespace Arena

#endif // Arena_ReadHandles_h
