PLUGINNAME = ReadCDR

PLUGINOBJECTS = \
    ReadCDR.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    fr.inria.openfx.ReadCDR.png \
    fr.inria.openfx.ReadCDR.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

CXXFLAGS  += \
    $(CDR_CXXFLAGS) \
    $(SVG_CXXFLAGS)
LINKFLAGS += \
    $(CDR_LINKFLAGS) \
    $(SVG_LINKFLAGS)

