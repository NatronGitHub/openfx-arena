PLUGINOBJECTS = \
    ReadXCF.o \
    PluginRegistration.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadXCF

RESOURCES = net.fxarena.openfx.ReadXCF.png net.fxarena.openfx.ReadXCF.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
