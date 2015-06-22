PLUGINOBJECTS = \
    ReadMisc.o \
    PluginRegistration.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadMisc

RESOURCES = net.fxarena.openfx.ReadMisc.png net.fxarena.openfx.ReadMisc.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
