PLUGINOBJECTS = \
	MagickText.o \
	MagickSwirl.o \
	MagickModulate.o \
	MagickMirror.o \
	MagickImplode.o \
	MagickEdge.o \
	ofxsOGLTextRenderer.o \
	ofxsOGLFontData.o \
	ofxsGenerator.o \
	ofxsRectangleInteract.o \
	PluginRegistrationCombined.o

PLUGINNAME = Arena

RESOURCES = \
../Magick/net.fxarena.openfx.MagickText.png \
../Magick/net.fxarena.openfx.MagickText.svg \
../Magick/net.fxarena.openfx.MagickSwirl.png \
../Magick/net.fxarena.openfx.MagickSwirl.svg \
../Magick/net.fxarena.openfx.MagickModulate.png \
../Magick/net.fxarena.openfx.MagickModulate.svg \
../Magick/net.fxarena.openfx.MagickMirror.png \
../Magick/net.fxarena.openfx.MagickMirror.svg \
../Magick/net.fxarena.openfx.MagickImplode.png \
../Magick/net.fxarena.openfx.MagickImplode.svg \
../Magick/net.fxarena.openfx.MagickEdge.png \
../Magick/net.fxarena.openfx.MagickEdge.svg \

VPATH += \
../Magick \

include ../Makefile.master

CXXFLAGS += -I../Magick

CXXFLAGS  += $(FONTCONFIG_CXXFLAGS) $(MAGICK_CXXFLAGS)
LINKFLAGS += $(FONTCONFIG_LINKFLAGS) $(MAGICK_LINKFLAGS)
