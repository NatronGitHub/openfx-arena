PLUGINOBJECTS = \
    ReadCDR.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadCDR

RESOURCES = fr.inria.openfx.ReadCDR.png fr.inria.openfx.ReadCDR.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS) $(CDR_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS) $(CDR_LINKFLAGS)

