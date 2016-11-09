PLUGINNAME = ReadPSD

PLUGINOBJECTS = \
    ReadPSD.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    net.fxarena.openfx.ReadPSD.png \
    net.fxarena.openfx.ReadPSD.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

CXXFLAGS  += \
    $(MAGICK_CXXFLAGS) \
    $(LCMS_CXXFLAGS)
LINKFLAGS += \
    $(MAGICK_LINKFLAGS) \
    $(LCMS_LINKFLAGS)

