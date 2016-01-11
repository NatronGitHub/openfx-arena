PLUGINOBJECTS = \
    ReadMVG.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadMVG

RESOURCES = net.fxarena.openfx.ReadMVG.png net.fxarena.openfx.ReadMVG.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
