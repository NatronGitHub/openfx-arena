PLUGINOBJECTS = \
    ReadPDF.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadPDF

RESOURCES = fr.inria.openfx.ReadPDF.png fr.inria.openfx.ReadPDF.svg

include ../Makefile.master
include ../Makefile.io

# cairo
CXXFLAGS += $(shell pkg-config cairo --cflags)
LINKFLAGS += $(shell pkg-config cairo --libs --static)

# poppler
CXXFLAGS += $(shell pkg-config poppler-glib --cflags)
LINKFLAGS += $(shell pkg-config poppler-glib --libs --static)

