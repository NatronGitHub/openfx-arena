PLUGINOBJECTS = \
    OpenRaster.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = OpenRaster

RESOURCES = fr.inria.openfx.OpenRaster.png fr.inria.openfx.OpenRaster.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS) $(XML_CXXFLAGS) $(ZIP_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS) $(XML_LINKFLAGS) $(ZIP_LINKFLAGS)
