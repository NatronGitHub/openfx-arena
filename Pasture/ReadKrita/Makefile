PLUGINOBJECTS = \
    ReadKrita.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadKrita

RESOURCES = fr.inria.openfx.ReadKrita.png fr.inria.openfx.ReadKrita.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS) $(XML_CXXFLAGS) $(ZIP_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS) $(XML_LINKFLAGS) $(ZIP_LINKFLAGS)
