PLUGINOBJECTS = \
        Mirror.o \
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
        Emboss.o \
        Charcoal.o \
        Oilpaint.o \
        TextPango.o \
        ReadPSD.o \
        ofxsOGLTextRenderer.o \
        ofxsOGLFontData.o \
        ofxsRectangleInteract.o \
        PluginRegistrationCombined.o \
        GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = Arena

RESOURCES = \
../Mirror/net.fxarena.openfx.Mirror.png \
../Mirror/net.fxarena.openfx.Mirror.svg \
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
../Emboss/net.fxarena.openfx.Emboss.png \
../Emboss/net.fxarena.openfx.Emboss.svg \
../Charcoal/net.fxarena.openfx.Charcoal.png \
../Charcoal/net.fxarena.openfx.Charcoal.svg \
../Oilpaint/net.fxarena.openfx.Oilpaint.png \
../Oilpaint/net.fxarena.openfx.Oilpaint.svg \
../TextPango/net.fxarena.openfx.TextPango.png \
../TextPango/net.fxarena.openfx.TextPango.svg \
../ReadPSD/net.fxarena.openfx.ReadPSD.png \
../ReadPSD/net.fxarena.openfx.ReadPSD.svg \

VPATH += \
../Mirror \
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
../Emboss \
../Charcoal \
../Oilpaint \
../TextPango \
../ReadPSD \

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += \
-I../Mirror \
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
-I../Emboss \
-I../Charcoal \
-I../Oilpaint \
-I../TextPango \
-I../ReadPSD \

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
