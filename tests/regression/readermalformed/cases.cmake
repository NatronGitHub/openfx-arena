# The cases of reader_malformed (tests/common/reader_malformed.cmake, which defines fails()): the
# readers on malformed or missing files, each a past bug (the issue is given with each file).
# fails(<identifier> <major> <file> <regex of the expected message>)
# The bad_ files, which make_samples.cpp writes:
# - bad_nosize.kra/.ora: no image size in the XML. getFrameBounds() returned true with the bounds
#   unset, and GenericReader made the RoD from an uninitialized rectangle (#129).
# - bad_hugexml, bad_hugepng: an XML or PNG entry whose size in the archive (2^64-1 or 2^63-1) is
#   far larger than its contents. The readers allocated that size (new char[size + 1] wrapped
#   around to new char[0] for 2^64-1, then zip_fread() wrote past it) (#128).
# - bad_hugecanvas.ora: a 200000000x200000000 canvas in stack.xml, which the host would allocate (#214).
# - bad_pngbomb.kra: a merged image whose PNG header says 65535x65535 (17 GB of RGBA) (#159).
# - bad_huge.svg: a canvas too large for an int or for cairo (#158).
# - bad_corrupt: not a zip file (or BMP file) at all; bad_empty.svg: an empty canvas (#129);
#   missing.*: no such file.
fails(fr.inria.openfx.ReadKrita 2 bad_nosize.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 bad_hugexml.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 bad_hugepng.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 bad_pngbomb.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 bad_corrupt.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 missing.kra "Cannot load frame|No filename")
fails(fr.inria.openfx.OpenRaster 2 bad_nosize.ora "Unable to read image")
fails(fr.inria.openfx.OpenRaster 2 bad_hugecanvas.ora "Unable to read image")
fails(fr.inria.openfx.OpenRaster 2 bad_hugexml.ora "Unable to read image|Empty and/or corrupt image")
fails(fr.inria.openfx.OpenRaster 2 bad_hugepng.ora "Unable to read image")
fails(fr.inria.openfx.OpenRaster 2 bad_corrupt.ora "Unable to read image|Empty and/or corrupt image")
fails(fr.inria.openfx.OpenRaster 2 missing.ora "Cannot load frame|No filename")
fails(net.fxarena.openfx.ReadSVG 4 bad_empty.svg "Empty or too large SVG canvas")
fails(net.fxarena.openfx.ReadSVG 3 bad_empty.svg "Empty or too large SVG canvas")
fails(net.fxarena.openfx.ReadSVG 4 bad_huge.svg "Empty or too large SVG canvas")
fails(net.fxarena.openfx.ReadSVG 3 bad_huge.svg "Empty or too large SVG canvas")
fails(net.fxarena.openfx.ReadSVG 4 missing.svg "Cannot load frame|No filename")
fails(fr.inria.openfx.ReadMisc 1 bad_corrupt.bmp ".")
fails(fr.inria.openfx.ReadMisc 1 missing.bmp "Cannot load frame|No filename")
if(READPDF)
  fails(fr.inria.openfx.ReadPDF 1 missing.pdf "Cannot load frame|No filename")
endif()
