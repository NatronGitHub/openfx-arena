PLUGINOBJECTS = \
        Text.o \
        Swirl.o \
        Mirror.o \
        Implode.o \
        Tile.o \
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

VPATH += \
../Transform \
../Image \
../Filter \

include ../Makefile.master

CXXFLAGS += -I../Transform -I../Image -I../Filter

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
