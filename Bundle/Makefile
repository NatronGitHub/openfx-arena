PLUGINOBJECTS = \
        Distort.o \
        Mirror.o \
        Texture.o \
        Tile.o \
        Reflection.o \
        Text.o \
        Arc.o \
        Polar.o \
        Roll.o \
        Wave.o \
        ofxsOGLTextRenderer.o \
        ofxsOGLFontData.o \
        ofxsRectangleInteract.o \
        PluginRegistrationCombined.o \

PLUGINNAME = Arena

RESOURCES = \
../Distort/net.fxarena.openfx.Distort.png \
../Distort/net.fxarena.openfx.Distort.svg \
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

VPATH += \
../Mirror \
../Distort \
../Texture \
../Tile \
../Reflection \
../Text \
../Arc \
../Polar \
../Roll \
../Wave \

include ../Makefile.master

CXXFLAGS += \
-I../Mirror \
-I../Distort \
-I../Texture \
-I../Tile \
-I../Reflection \
-I../Text \
-I../Arc \
-I../Polar \
-I../Roll \
-I../Wave \

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
