PLUGINOBJECTS = \
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
        ofxsOGLTextRenderer.o \
        ofxsOGLFontData.o \
        ofxsRectangleInteract.o \
        GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = Magick

RESOURCES = \
../Bundle/Extra.png \
../Bundle/Extra.svg \
../Texture/net.fxarena.openfx.Texture.png \
../Texture/net.fxarena.openfx.Texture.svg \
../Tile/net.fxarena.openfx.Tile.png \
../Tile/net.fxarena.openfx.Tile.svg \
../Reflection/net.fxarena.openfx.Reflection.png \
../Reflection/net.fxarena.openfx.Reflection.svg \
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
../ReadPSD/net.fxarena.openfx.ReadPSD.png \
../ReadPSD/net.fxarena.openfx.ReadPSD.svg \
../Sketch/net.fxarena.openfx.Sketch.png \
../Sketch/net.fxarena.openfx.Sketch.svg \
../Polaroid/net.fxarena.openfx.Polaroid.png \
../Polaroid/net.fxarena.openfx.Polaroid.svg \
../Edges/net.fxarena.openfx.Edges.png \
../Edges/net.fxarena.openfx.Edges.svg \
../Modulate/net.fxarena.openfx.Modulate.png \
../Modulate/net.fxarena.openfx.Modulate.svg \
../ReadMisc/fr.inria.openfx.ReadMisc.png \
../ReadMisc/fr.inria.openfx.ReadMisc.svg \
../Text/net.fxarena.openfx.Text.png \
../Text/net.fxarena.openfx.Text.svg

VPATH += \
../Texture \
../Tile \
../Reflection \
../Arc \
../Polar \
../Roll \
../Wave \
../Swirl \
../Implode \
../Charcoal \
../Oilpaint \
../ReadPSD \
../Sketch \
../Polaroid \
../Edges \
../Modulate \
../ReadMisc \
../Text

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += \
-I../Texture \
-I../Tile \
-I../Reflection \
-I../Arc \
-I../Polar \
-I../Roll \
-I../Wave \
-I../Swirl \
-I../Implode \
-I../Charcoal \
-I../Oilpaint \
-I../ReadPSD \
-I../Sketch \
-I../Polaroid \
-I../Edges \
-I../Modulate \
-I../ReadMisc \
-I../Text

CXXFLAGS  += $(MAGICK_CXXFLAGS) $(LCMS_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS) $(LCMS_LINKFLAGS)

