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

// Reading the entries of the zip archives of ReadKrita (.kra) and OpenRaster (.ora).
//
// The entry sizes come from the archive, i.e. from the file, which may be malformed or hostile
// (#128): a size is used only if libzip says it is valid, and only up to a cap, so that it can't
// wrap around (a zip64 size of 2^64-1 gave new char[0]) or ask for an unbounded allocation.

#ifndef Arena_ReadZip_h
#define Arena_ReadZip_h

#include <string>
#include <zip.h>

#include "ReadHandles.h"

namespace ArenaZip {

// the largest entries read: the XML documents (maindoc.xml, stack.xml) and the PNG images
static const zip_uint64_t kMaxXMLSize = zip_uint64_t(64) << 20; // 64 MiB
static const zip_uint64_t kMaxImageSize = zip_uint64_t(1) << 30; // 1 GiB

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

} // namespace ArenaZip

#endif // Arena_ReadZip_h
