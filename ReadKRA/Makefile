PLUGINOBJECTS = \
    ReadKRA.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadKRA

RESOURCES = fr.inria.openfx.ReadKRA.png fr.inria.openfx.ReadKRA.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS) $(XML_CXXFLAGS) $(ZIP_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS) $(XML_LINKFLAGS) $(ZIP_LINKFLAGS)
