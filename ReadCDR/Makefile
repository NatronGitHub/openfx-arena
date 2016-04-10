PLUGINOBJECTS = \
    ReadCDR.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadCDR

RESOURCES = fr.inria.openfx.ReadCDR.png fr.inria.openfx.ReadCDR.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(SVG_CXXFLAGS) $(CDR_CXXFLAGS) $(REVENGE_CXXFLAGS)
LINKFLAGS += $(SVG_LINKFLAGS) $(CDR_LINKFLAGS) $(REVENGE_LINKFLAGS)

