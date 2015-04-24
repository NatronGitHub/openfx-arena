PLUGINOBJECTS = \
	MagickText.o \
	MagickSwirl.o \
	MagickMirror.o \
	MagickImplode.o \
	MagickTile.o \
        ofxsOGLTextRenderer.o \
        ofxsOGLFontData.o \
        ofxsGenerator.o \
        ofxsRectangleInteract.o \
	PluginRegistration.o \

PLUGINNAME = Magick

RESOURCES = \
net.fxarena.openfx.MagickText.png \
net.fxarena.openfx.MagickText.svg \
net.fxarena.openfx.MagickSwirl.png \
net.fxarena.openfx.MagickSwirl.svg \
net.fxarena.openfx.MagickMirror.png \
net.fxarena.openfx.MagickMirror.svg \
net.fxarena.openfx.MagickImplode.png \
net.fxarena.openfx.MagickImplode.svg \
net.fxarena.openfx.MagickTile.png \
net.fxarena.openfx.MagickTile.svg \

include ../Makefile.master

CXXFLAGS += $(FONTCONFIG_CXXFLAGS) $(MAGICK_CXXFLAGS)
LINKFLAGS += $(FONTCONFIG_LINKFLAGS) $(MAGICK_LINKFLAGS)
