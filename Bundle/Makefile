PLUGINOBJECTS = \
        Text.o \
        Distort.o \
        Mirror.o \
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
../Mirror/net.fxarena.openfx.Mirror.png \
../Mirror/net.fxarena.openfx.Mirror.svg \

VPATH += \
../Mirror \
../Text \
../Distort \

include ../Makefile.master

CXXFLAGS += -I../Mirror -I../Text -I../Distort

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
