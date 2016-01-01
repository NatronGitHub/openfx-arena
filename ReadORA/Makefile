PLUGINOBJECTS = \
    ReadORA.o \
    PluginRegistration.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadORA

RESOURCES = fr.inria.openfx.ReadORA.png fr.inria.openfx.ReadORA.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += $(MAGICK_CXXFLAGS) $(XML_CXXFLAGS) $(ZIP_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS) $(XML_LINKFLAGS) $(ZIP_LINKFLAGS)
