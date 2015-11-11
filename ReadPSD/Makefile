PLUGINOBJECTS = \
    ReadPSD.o \
    PluginRegistration.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadPSD

RESOURCES = net.fxarena.openfx.ReadPSD.png net.fxarena.openfx.ReadPSD.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS) $(LCMS_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS) $(LCMS_LINKFLAGS)
