PLUGINOBJECTS = \
        lodepng.o \
        TextFX.o \
        ReadCDR.o \
        ReadSVG.o \
        ReadKrita.o \
        OpenRaster.o

ifeq ($(LICENSE),GPL)
        PLUGINOBJECTS += ReadPDF.o
endif

PLUGINOBJECTS += \
        Texture.o \
        Tile.o \
        Reflection.o \
        Arc.o \
        Polar.o \
        Roll.o \
        Wave.o \
        Swirl.o \
        Implode.o \
        Charcoal.o \
        Oilpaint.o \
        ReadPSD.o \
        Sketch.o \
        Polaroid.o \
        Edges.o \
        Modulate.o \
        ReadMisc.o \
        Text.o \
        ofxsOGLTextRenderer.o ofxsOGLFontData.o ofxsRectangleInteract.o \
        GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = Arena

RESOURCES = \
../Extra/fr.inria.openfx.TextFX.png \
../Extra/fr.inria.openfx.TextFX.svg \
../Extra/fr.inria.openfx.ReadCDR.png \
../Extra/fr.inria.openfx.ReadCDR.svg \
../Extra/net.fxarena.openfx.ReadSVG.png \
../Extra/net.fxarena.openfx.ReadSVG.svg \
../Extra/fr.inria.openfx.ReadKrita.png \
../Extra/fr.inria.openfx.ReadKrita.svg \
../Extra/fr.inria.openfx.OpenRaster.png \
../Extra/fr.inria.openfx.OpenRaster.svg

ifeq ($(LICENSE),GPL)
        RESOURCES += fr.inria.openfx.ReadPDF.png fr.inria.openfx.ReadPDF.svg
endif

RESOURCES += \
../Magick/Extra.png \
../Magick/Extra.svg \
../Magick/net.fxarena.openfx.Texture.png \
../Magick/net.fxarena.openfx.Texture.svg \
../Magick/net.fxarena.openfx.Tile.png \
../Magick/net.fxarena.openfx.Tile.svg \
../Magick/net.fxarena.openfx.Reflection.png \
../Magick/net.fxarena.openfx.Reflection.svg \
../Magick/net.fxarena.openfx.Arc.png \
../Magick/net.fxarena.openfx.Arc.svg \
../Magick/net.fxarena.openfx.Polar.png \
../Magick/net.fxarena.openfx.Polar.svg \
../Magick/net.fxarena.openfx.Roll.png \
../Magick/net.fxarena.openfx.Roll.svg \
../Magick/net.fxarena.openfx.Wave.png \
../Magick/net.fxarena.openfx.Wave.svg \
../Magick/net.fxarena.openfx.Swirl.png \
../Magick/net.fxarena.openfx.Swirl.svg \
../Magick/net.fxarena.openfx.Implode.png \
../Magick/net.fxarena.openfx.Implode.svg \
../Magick/net.fxarena.openfx.Charcoal.png \
../Magick/net.fxarena.openfx.Charcoal.svg \
../Magick/net.fxarena.openfx.Oilpaint.png \
../Magick/net.fxarena.openfx.Oilpaint.svg \
../Magick/net.fxarena.openfx.ReadPSD.png \
../Magick/net.fxarena.openfx.ReadPSD.svg \
../Magick/net.fxarena.openfx.Sketch.png \
../Magick/net.fxarena.openfx.Sketch.svg \
../Magick/net.fxarena.openfx.Polaroid.png \
../Magick/net.fxarena.openfx.Polaroid.svg \
../Magick/net.fxarena.openfx.Edges.png \
../Magick/net.fxarena.openfx.Edges.svg \
../Magick/net.fxarena.openfx.Modulate.png \
../Magick/net.fxarena.openfx.Modulate.svg \
../Magick/fr.inria.openfx.ReadMisc.png \
../Magick/fr.inria.openfx.ReadMisc.svg \
../Magick/net.fxarena.openfx.Text.png \
../Magick/net.fxarena.openfx.Text.svg

VPATH += \
../Extra \
../Magick

CXXFLAGS += \
-I../Extra \
-I../Magick

include ../Makefile.master
include ../Makefile.io

CXXFLAGS  += $(FCONFIG_CXXFLAGS) $(PANGO_CXXFLAGS) $(SVG_CXXFLAGS) $(CDR_CXXFLAGS) $(XML_CXXFLAGS) $(ZIP_CXXFLAGS) $(MAGICK_CXXFLAGS) $(LCMS_CXXFLAGS)
LINKFLAGS += $(FCONFIG_LINKFLAGS) $(PANGO_LINKFLAGS) $(SVG_LINKFLAGS) $(CDR_LINKFLAGS) $(XML_LINKFLAGS) $(ZIP_LINKFLAGS) $(MAGICK_LINKFLAGS) $(LCMS_LINKFLAGS)

ifeq ($(LICENSE),GPL)
        CXXFLAGS += $(POPPLER_CXXFLAGS)
        LINKFLAGS += $(POPPLER_LINKFLAGS)
endif

ifeq ($(IM),7)
        CXXFLAGS += -DIM7
endif

PNGVERSION = a70c086077c0eaecbae3845e4da4424de5f43361

lodepng.cpp:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.cpp

lodepng.h:
	curl -o $@ https://raw.githubusercontent.com/lvandeve/lodepng/$(PNGVERSION)/lodepng.h

$(OBJECTPATH)/lodepng.o: lodepng.cpp lodepng.h
$(OBJECTPATH)/ReadKrita.o: ReadKrita.cpp lodepng.h
$(OBJECTPATH)/OpenRaster.o: OpenRaster.cpp lodepng.h
