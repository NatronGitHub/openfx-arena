PLUGINNAME = ReadKrita

PLUGINOBJECTS = \
    lodepng.o \
    ReadKrita.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    fr.inria.openfx.ReadKrita.png \
    fr.inria.openfx.ReadKrita.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io


CXXFLAGS  += \
    $(XML_CXXFLAGS) \
    $(ZIP_CXXFLAGS)
LINKFLAGS += \
    $(XML_LINKFLAGS) \
    $(ZIP_LINKFLAGS)

PNGVERSION = a70c086077c0eaecbae3845e4da4424de5f43361

lodepng.cpp:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.cpp

lodepng.h:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.h

$(OBJECTPATH)/lodepng.o: lodepng.cpp lodepng.h
$(OBJECTPATH)/ReadKrita.o: ReadKrita.cpp lodepng.h
