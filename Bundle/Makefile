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
../Filter/net.fxarena.openfx.Swirl.png \
../Filter/net.fxarena.openfx.Swirl.svg \
../Magick/net.fxarena.openfx.MagickMirror.png \
../Magick/net.fxarena.openfx.MagickMirror.svg \
../Filter/net.fxarena.openfx.Implode.png \
../Filter/net.fxarena.openfx.Implode.svg \
../Magick/net.fxarena.openfx.MagickTile.png \
../Magick/net.fxarena.openfx.MagickTile.svg \

VPATH += \
../Magick \
../Image \
../Filter \

include ../Makefile.master

CXXFLAGS += -I../Magick -I../Image -I../Filter

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
