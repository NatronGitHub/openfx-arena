PLUGINNAME = Extra

PLUGINOBJECTS = \
    lodepng.o \
    TextFX.o \
    ReadSVG.o \
    ReadKrita.o \
    OpenRaster.o \
    ofxsTransform3x3.o \
    ofxsTransformInteract.o \
    ofxsShutter.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    net.fxarena.openfx.Text.png \
    net.fxarena.openfx.Text.svg \
    net.fxarena.openfx.ReadSVG.png \
    net.fxarena.openfx.ReadSVG.svg \
    fr.inria.openfx.ReadKrita.png \
    fr.inria.openfx.ReadKrita.svg \
    fr.inria.openfx.OpenRaster.png \
    fr.inria.openfx.OpenRaster.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

CXXFLAGS  += \
    $(FCONFIG_CXXFLAGS) \
    $(PANGO_CXXFLAGS) \
    $(SVG_CXXFLAGS) \
    $(CDR_CXXFLAGS) \
    $(XML_CXXFLAGS) \
    $(ZIP_CXXFLAGS) \
    $(GLIB_CXXFLAGS) \
    $(POPPLER_CXXFLAGS)
LINKFLAGS += \
    $(FCONFIG_LINKFLAGS) \
    $(PANGO_LINKFLAGS) \
    $(SVG_LINKFLAGS) \
    $(CDR_LINKFLAGS) \
    $(XML_LINKFLAGS) \
    $(ZIP_LINKFLAGS) \
    $(GLIB_LINKFLAGS) \
    $(POPPLER_LINKFLAGS)

PNGVERSION = a70c086077c0eaecbae3845e4da4424de5f43361

lodepng.cpp:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.cpp

lodepng.h:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.h

$(OBJECTPATH)/lodepng.o: lodepng.cpp lodepng.h
$(OBJECTPATH)/ReadKrita.o: ReadKrita.cpp lodepng.h
$(OBJECTPATH)/OpenRaster.o: OpenRaster.cpp lodepng.h
