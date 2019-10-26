PLUGINNAME = Extra

PLUGINOBJECTS = \
    lodepng.o \
    ReadCDR.o \
    ReadSVG.o \
    ReadKrita.o \
    OpenRaster.o

ifneq ($(LICENSE),COMMERCIAL)
PLUGINOBJECTS += ReadPDF.o
endif

PLUGINOBJECTS += \
    ofxsTransform3x3.o \
    ofxsTransformInteract.o \
    ofxsShutter.o \
    ofxsMultiPlane.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    fr.inria.openfx.ReadCDR.png \
    fr.inria.openfx.ReadCDR.svg \
    net.fxarena.openfx.ReadSVG.png \
    net.fxarena.openfx.ReadSVG.svg \
    fr.inria.openfx.ReadKrita.png \
    fr.inria.openfx.ReadKrita.svg \
    fr.inria.openfx.OpenRaster.png \
    fr.inria.openfx.OpenRaster.svg

ifneq ($(LICENSE),COMMERCIAL)
RESOURCES += \
    fr.inria.openfx.ReadPDF.png \
    fr.inria.openfx.ReadPDF.svg
endif

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

CXXFLAGS  += \
    $(FCONFIG_CXXFLAGS) \
    $(SVG_CXXFLAGS) \
    $(CDR_CXXFLAGS) \
    $(XML_CXXFLAGS) \
    $(ZIP_CXXFLAGS) \
    $(GLIB_CXXFLAGS)
LINKFLAGS += \
    $(FCONFIG_LINKFLAGS) \
    $(SVG_LINKFLAGS) \
    $(CDR_LINKFLAGS) \
    $(XML_LINKFLAGS) \
    $(ZIP_LINKFLAGS) \
    $(GLIB_LINKFLAGS)

ifneq ($(LICENSE),COMMERCIAL)
CXXFLAGS += $(POPPLER_CXXFLAGS)
LINKFLAGS += $(POPPLER_LINKFLAGS)
endif

PNGVERSION = a70c086077c0eaecbae3845e4da4424de5f43361

lodepng.cpp:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.cpp

lodepng.h:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.h

$(OBJECTPATH)/lodepng.o: lodepng.cpp lodepng.h
$(OBJECTPATH)/ReadKrita.o: ReadKrita.cpp lodepng.h
$(OBJECTPATH)/OpenRaster.o: OpenRaster.cpp lodepng.h
