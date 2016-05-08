PLUGINOBJECTS = \
        lodepng.o \
        TextFX.o \
        ReadCDR.o \
        ReadSVG.o \
        ReadKrita.o \
        OpenRaster.o \
        ofxsOGLTextRenderer.o ofxsOGLFontData.o ofxsRectangleInteract.o \
        GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = Extra

RESOURCES = \
fr.inria.openfx.TextFX.png \
fr.inria.openfx.TextFX.svg \
fr.inria.openfx.ReadCDR.png \
fr.inria.openfx.ReadCDR.svg \
net.fxarena.openfx.ReadSVG.png \
net.fxarena.openfx.ReadSVG.svg \
fr.inria.openfx.ReadKrita.png \
fr.inria.openfx.ReadKrita.svg \
fr.inria.openfx.OpenRaster.png \
fr.inria.openfx.OpenRaster.svg

include ../Makefile.master
include ../Makefile.io

CXXFLAGS  += $(FCONFIG_CXXFLAGS) $(PANGO_CXXFLAGS) $(SVG_CXXFLAGS) $(CDR_CXXFLAGS) $(XML_CXXFLAGS) $(ZIP_CXXFLAGS)
LINKFLAGS += $(FCONFIG_LINKFLAGS) $(PANGO_LINKFLAGS) $(SVG_LINKFLAGS) $(CDR_LINKFLAGS) $(XML_LINKFLAGS) $(ZIP_LINKFLAGS)

PNGVERSION = a70c086077c0eaecbae3845e4da4424de5f43361

lodepng.cpp:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.cpp

lodepng.h:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.h

$(OBJECTPATH)/lodepng.o: lodepng.cpp lodepng.h
