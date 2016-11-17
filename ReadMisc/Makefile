PLUGINNAME = ReadMisc

PLUGINOBJECTS = \
    ReadMisc.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    fr.inria.openfx.ReadMisc.png \
    fr.inria.openfx.ReadMisc.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

CXXFLAGS  += \
    $(MAGICK_CXXFLAGS)
LINKFLAGS += \
    $(MAGICK_LINKFLAGS)

