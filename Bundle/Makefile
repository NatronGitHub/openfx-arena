PLUGINOBJECTS = \
        Text.o \
	MagickSwirl.o \
	MagickMirror.o \
	MagickImplode.o \
	MagickTile.o \
	ofxsOGLTextRenderer.o \
	ofxsOGLFontData.o \
	ofxsRectangleInteract.o \
	PluginRegistrationCombined.o \

PLUGINNAME = Arena

RESOURCES = \
../Image/net.fxarena.openfx.Text.png \
../Image/net.fxarena.openfx.Text.svg \
../Magick/net.fxarena.openfx.MagickSwirl.png \
../Magick/net.fxarena.openfx.MagickSwirl.svg \
../Magick/net.fxarena.openfx.MagickMirror.png \
../Magick/net.fxarena.openfx.MagickMirror.svg \
../Magick/net.fxarena.openfx.MagickImplode.png \
../Magick/net.fxarena.openfx.MagickImplode.svg \
../Magick/net.fxarena.openfx.MagickTile.png \
../Magick/net.fxarena.openfx.MagickTile.svg \

VPATH += \
../Magick \
../Image \

include ../Makefile.master

CXXFLAGS += -I../Magick -I../Image

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
