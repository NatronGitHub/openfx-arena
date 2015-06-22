PLUGINOBJECTS = \
    ReadSVG.o \
    PluginRegistration.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadSVG

RESOURCES = net.fxarena.openfx.ReadSVG.png net.fxarena.openfx.ReadSVG.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
