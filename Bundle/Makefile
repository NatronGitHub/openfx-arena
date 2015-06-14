PLUGINOBJECTS = \
        Distort.o \
        Mirror.o \
        Texture.o \
        Tile.o \
        Reflection.o \
        Text.o \
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

VPATH += \
../Mirror \
../Distort \
../Texture \
../Tile \
../Reflection \
../Text \

include ../Makefile.master

CXXFLAGS += -I../Mirror -I../Distort -I../Texture -I../Tile -I../Reflection -I../Text

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
