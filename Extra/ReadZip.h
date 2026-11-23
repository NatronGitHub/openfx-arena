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

// Reading the entries of the zip archives of ReadKrita (.kra) and OpenRaster (.ora), and their
// XML and PNG contents.
//
// The entry sizes come from the archive, i.e. from the file, which may be malformed or hostile
// (#128): a size is used only if libzip says it is valid, and only up to a cap, so that it can't
// wrap around (a zip64 size of 2^64-1 gave new char[0]) or ask for an unbounded allocation.

#ifndef Arena_ReadZip_h
#define Arena_ReadZip_h

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <libxml/parser.h>
#include <zip.h>

#include "lodepng.h"
#include "ReadHandles.h"

namespace ArenaZip {

// the largest entries read: the XML documents (maindoc.xml, stack.xml) and the PNG images
constexpr zip_uint64_t kMaxXMLSize = zip_uint64_t(64) << 20; // 64 MiB
constexpr zip_uint64_t kMaxImageSize = zip_uint64_t(1) << 30; // 1 GiB

// The size of the entry name of an open archive, or 0 if it is missing, empty, or its size is
// unknown.
inline zip_uint64_t
entrySize(zip* archive, const char* name)
{
    if (!archive) {
        return 0;
    }
    struct zip_stat st;
    zip_stat_init(&st);
    if (zip_stat(archive, name, 0, &st) != 0 || !(st.valid & ZIP_STAT_SIZE)) {
        return 0;
    }
    return st.size;
}

// The entry name of an open archive, or "" if it is missing, empty, larger than maxSize, or
// can't be read.
inline std::string
readEntry(zip* archive, const char* name, zip_uint64_t maxSize)
{
    std::string output;
    const zip_uint64_t size = entrySize(archive, name);
    if (size == 0 || size > maxSize) {
        return output;
    }
    Arena::ZipFilePtr file(zip_fopen(archive, name, 0));
    if (!file) {
        return output;
    }
    std::string data(static_cast<size_t>(size), '\0');
    const zip_int64_t nread = zip_fread(file.get(), &data[0], size);
    if (nread >= 0 && static_cast<zip_uint64_t>(nread) == size) {
        output.swap(data);
    }
    return output;
}

// The archive file filename, opened read-only: NULL if it is missing or corrupt.
inline Arena::ZipPtr
openArchive(const std::string& filename)
{
    int err = 0;
    return Arena::ZipPtr(zip_open(filename.c_str(), ZIP_RDONLY, &err));
}

// The entry name of the archive file filename, as readEntry().
inline std::string
readEntry(const std::string& filename, const char* name, zip_uint64_t maxSize)
{
    const Arena::ZipPtr archive = openArchive(filename);
    return readEntry(archive.get(), name, maxSize); // readEntry(NULL, ...) is ""
}

// An XML entry, parsed: NULL if it is malformed. No network access, and no messages on stderr
// for a malformed file (#160). xml comes from readEntry(), so its size fits in an int.
inline xmlDocPtr
parseXML(const std::string& xml)
{
    return xmlReadMemory(xml.data(), static_cast<int>(xml.size()), NULL, NULL,
                         XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
}

// An integer attribute (a size or an offset), 0 if s is NULL, clamped to +/-2^28 so that the
// sums of sizes and offsets can't overflow an int (#158).
inline int
parseInt(const xmlChar* s)
{
    if (!s) {
        return 0;
    }
    const long kMax = 1L << 28;
    const long v = std::strtol(reinterpret_cast<const char*>(s), NULL, 10); // LONG_MIN/MAX on overflow
    return static_cast<int>(std::max(-kMax, std::min(v, kMax)));
}

// the largest decoded image, in bytes of 8-bit RGBA (#159)
constexpr std::uint64_t kMaxDecodedSize = std::uint64_t(1) << 32; // 4 GiB

// A PNG entry, decoded to 8-bit RGBA as lodepng_decode32() does: NULL if it can't be decoded, or
// if its header size is over kMaxDecodedSize (a small PNG may ask for 17 GB, #159). The row bytes
// (w * 4) fit in an int.
inline Arena::MallocPtr<unsigned char>
decodePNG(const std::string& png, unsigned* w, unsigned* h)
{
    Arena::MallocPtr<unsigned char> image;
    const unsigned char* in = reinterpret_cast<const unsigned char*>(png.data());
    LodePNGState state;
    lodepng_state_init(&state); // the output is 8-bit RGBA by default
    unsigned err = lodepng_inspect(w, h, &state, in, png.size());
    if (!err && *w <= INT_MAX / 4 && std::uint64_t(*w) * *h * 4 <= kMaxDecodedSize) {
        // the zlib stream may still decode to more than the header says: at most twice the image
        // (16-bit RGBA), plus a filter byte per row
        state.decoder.zlibsettings.max_output_size =
            static_cast<size_t>(std::min<std::uint64_t>(2 * kMaxDecodedSize + *h, SIZE_MAX));
        unsigned char* out = NULL;
        err = lodepng_decode(&out, w, h, &state, in, png.size());
        image.reset(out); // lodepng may allocate the output even on error
        if (err) {
            image.reset();
        }
    }
    lodepng_state_cleanup(&state);
    return image;
}

} // namespace ArenaZip

#endif // Arena_ReadZip_h
