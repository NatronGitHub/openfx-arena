PLUGINOBJECTS = \
        Text.o \
        Distort.o \
        Mirror.o \
        Tile.o \
        Reflection.o \
	ofxsOGLTextRenderer.o \
	ofxsOGLFontData.o \
	ofxsRectangleInteract.o \
	PluginRegistrationCombined.o \

PLUGINNAME = Arena

RESOURCES = \
../Text/net.fxarena.openfx.Text.png \
../Text/net.fxarena.openfx.Text.svg \
../Distort/net.fxarena.openfx.Distort.png \
../Distort/net.fxarena.openfx.Distort.svg \
../Transform/net.fxarena.openfx.Mirror.png \
../Transform/net.fxarena.openfx.Mirror.svg \
../Transform/net.fxarena.openfx.Tile.png \
../Transform/net.fxarena.openfx.Tile.svg \
../Transform/net.fxarena.openfx.Reflection.png \
../Transform/net.fxarena.openfx.Reflection.svg \

VPATH += \
../Transform \
../Text \
../Distort \

include ../Makefile.master

CXXFLAGS += -I../Transform -I../Text -I../Distort

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
