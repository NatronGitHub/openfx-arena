PLUGINOBJECTS = \
        Texture.o \
        Tile.o \
        Reflection.o \
        Text.o \
        Arc.o \
        Polar.o \
        Roll.o \
        Wave.o \
        Swirl.o \
        Implode.o \
        Charcoal.o \
        Oilpaint.o \
        TextPango.o \
        ReadPSD.o \
        ReadSVG.o \
        Sketch.o \
        Polaroid.o \
        Edges.o \
        Modulate.o \
        OpenRaster.o \
        ReadKrita.o \
        ReadMisc.o \
        ReadCDR.o \
        ofxsOGLTextRenderer.o \
        ofxsOGLFontData.o \
        ofxsRectangleInteract.o \
        GenericReader.o GenericOCIO.o SequenceParsing.o

ifeq ($(OS),$(filter $(OS),Linux FreeBSD Darwin))
    PLUGINOBJECTS += PovRay.o
endif

PLUGINNAME = Arena

RESOURCES = \
../Bundle/Extra.png \
../Bundle/Extra.svg \
../Texture/net.fxarena.openfx.Texture.png \
../Texture/net.fxarena.openfx.Texture.svg \
../Tile/net.fxarena.openfx.Tile.png \
../Tile/net.fxarena.openfx.Tile.svg \
../Reflection/net.fxarena.openfx.Reflection.png \
../Reflection/net.fxarena.openfx.Reflection.svg \
../Text/net.fxarena.openfx.Text.png \
../Text/net.fxarena.openfx.Text.svg \
../Arc/net.fxarena.openfx.Arc.png \
../Arc/net.fxarena.openfx.Arc.svg \
../Polar/net.fxarena.openfx.Polar.png \
../Polar/net.fxarena.openfx.Polar.svg \
../Roll/net.fxarena.openfx.Roll.png \
../Roll/net.fxarena.openfx.Roll.svg \
../Wave/net.fxarena.openfx.Wave.png \
../Wave/net.fxarena.openfx.Wave.svg \
../Swirl/net.fxarena.openfx.Swirl.png \
../Swirl/net.fxarena.openfx.Swirl.svg \
../Implode/net.fxarena.openfx.Implode.png \
../Implode/net.fxarena.openfx.Implode.svg \
../Charcoal/net.fxarena.openfx.Charcoal.png \
../Charcoal/net.fxarena.openfx.Charcoal.svg \
../Oilpaint/net.fxarena.openfx.Oilpaint.png \
../Oilpaint/net.fxarena.openfx.Oilpaint.svg \
../TextPango/net.fxarena.openfx.TextPango.png \
../TextPango/net.fxarena.openfx.TextPango.svg \
../ReadPSD/net.fxarena.openfx.ReadPSD.png \
../ReadPSD/net.fxarena.openfx.ReadPSD.svg \
../ReadSVG/net.fxarena.openfx.ReadSVG.png \
../ReadSVG/net.fxarena.openfx.ReadSVG.svg \
../Sketch/net.fxarena.openfx.Sketch.png \
../Sketch/net.fxarena.openfx.Sketch.svg \
../Polaroid/net.fxarena.openfx.Polaroid.png \
../Polaroid/net.fxarena.openfx.Polaroid.svg \
../Edges/net.fxarena.openfx.Edges.png \
../Edges/net.fxarena.openfx.Edges.svg \
../Modulate/net.fxarena.openfx.Modulate.png \
../Modulate/net.fxarena.openfx.Modulate.svg \
../OpenRaster/fr.inria.openfx.OpenRaster.png \
../OpenRaster/fr.inria.openfx.OpenRaster.svg \
../ReadKrita/fr.inria.openfx.ReadKrita.png \
../ReadKrita/fr.inria.openfx.ReadKrita.svg \
../ReadMisc/fr.inria.openfx.ReadMisc.png \
../ReadMisc/fr.inria.openfx.ReadMisc.svg \
../ReadCDR/fr.inria.openfx.ReadCDR.png \
../ReadCDR/fr.inria.openfx.ReadCDR.svg

ifeq ($(OS),$(filter $(OS),Linux FreeBSD Darwin))
    RESOURCES +=../PovRay/net.fxarena.openfx.PovRay.png
    RESOURCES +=../PovRay/net.fxarena.openfx.PovRay.svg
endif

VPATH += \
../Texture \
../Tile \
../Reflection \
../Text \
../Arc \
../Polar \
../Roll \
../Wave \
../Swirl \
../Implode \
../Charcoal \
../Oilpaint \
../TextPango \
../ReadPSD \
../ReadSVG \
../Sketch \
../Polaroid \
../Edges \
../Modulate \
../OpenRaster \
../ReadKrita \
../ReadMisc \
../ReadCDR

ifeq ($(OS),$(filter $(OS),Linux FreeBSD Darwin))
    VPATH += ../PovRay
endif

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += \
-I../Texture \
-I../Tile \
-I../Reflection \
-I../Text \
-I../Arc \
-I../Polar \
-I../Roll \
-I../Wave \
-I../Swirl \
-I../Implode \
-I../Charcoal \
-I../Oilpaint \
-I../TextPango \
-I../ReadPSD \
-I../ReadSVG \
-I../Sketch \
-I../Polaroid \
-I../Edges \
-I../Modulate \
-I../OpenRaster \
-I../ReadKrita \
-I../ReadMisc \
-I../ReadCDR

ifeq ($(OS),$(filter $(OS),Linux FreeBSD Darwin))
    CXXFLAGS += -I../PovRay
endif

CXXFLAGS  += $(MAGICK_CXXFLAGS) $(LCMS_CXXFLAGS) $(FCONFIG_CXXFLAGS) $(XML_CXXFLAGS) $(ZIP_CXXFLAGS) $(SVG_CXXFLAGS) $(REVENGE_CXXFLAGS) $(CDR_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS) $(LCMS_LINKFLAGS) $(FCONFIG_LINKFLAGS) $(XML_LINKFLAGS) $(ZIP_LINKFLAGS) $(SVG_LINKFLAGS) $(REVENGE_LINKFLAGS) $(CDR_LINKFLAGS)

