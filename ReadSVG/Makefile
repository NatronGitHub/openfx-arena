PLUGINNAME = ReadSVG

PLUGINOBJECTS = \
    ReadSVG.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    net.fxarena.openfx.ReadSVG.png \
    net.fxarena.openfx.ReadSVG.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

CXXFLAGS  += \
    $(SVG_CXXFLAGS) \
    $(XML_CXXFLAGS)
LINKFLAGS += \
    $(SVG_LINKFLAGS) \
    $(XML_LINKFLAGS)

