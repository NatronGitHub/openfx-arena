PLUGINOBJECTS = \
        Text.o \
        Swirl.o \
        Mirror.o \
        Implode.o \
        Polar.o \
        Arc.o \
        Tile.o \
        Reflection.o \
	ofxsOGLTextRenderer.o \
	ofxsOGLFontData.o \
	ofxsRectangleInteract.o \
	PluginRegistrationCombined.o \

PLUGINNAME = Arena

RESOURCES = \
../Image/net.fxarena.openfx.Text.png \
../Image/net.fxarena.openfx.Text.svg \
../Filter/net.fxarena.openfx.Swirl.png \
../Filter/net.fxarena.openfx.Swirl.svg \
../Transform/net.fxarena.openfx.Mirror.png \
../Transform/net.fxarena.openfx.Mirror.svg \
../Filter/net.fxarena.openfx.Implode.png \
../Filter/net.fxarena.openfx.Implode.svg \
../Transform/net.fxarena.openfx.Tile.png \
../Transform/net.fxarena.openfx.Tile.svg \
../Transform/net.fxarena.openfx.Reflection.png \
../Transform/net.fxarena.openfx.Reflection.svg \
../Filter/net.fxarena.openfx.Polar.png \
../Filter/net.fxarena.openfx.Polar.svg \
../Filter/net.fxarena.openfx.Arc.png \
../Filter/net.fxarena.openfx.Arc.svg \

VPATH += \
../Transform \
../Image \
../Filter \

include ../Makefile.master

CXXFLAGS += -I../Transform -I../Image -I../Filter

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
