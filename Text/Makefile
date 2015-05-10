PLUGINOBJECTS = \
        Text.o \
	ofxsOGLTextRenderer.o \
	ofxsOGLFontData.o \
	ofxsRectangleInteract.o \
	PluginRegistrationCombined.o \

PLUGINNAME = Text

RESOURCES = \
../Image/net.fxarena.openfx.Text.png \
../Image/net.fxarena.openfx.Text.svg \

VPATH += \
../Image \

include ../Makefile.master

CXXFLAGS += -I../Image

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
