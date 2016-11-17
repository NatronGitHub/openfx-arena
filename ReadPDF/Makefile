PLUGINNAME = ReadPDF

PLUGINOBJECTS = \
    ReadPDF.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    $(SRCDIR)/ReadPDF/fr.inria.openfx.ReadPDF.png \
    $(SRCDIR)/ReadPDF/fr.inria.openfx.ReadPDF.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

CXXFLAGS += $(POPPLER_CXXFLAGS)
LINKFLAGS += $(POPPLER_LINKFLAGS)

