PLUGINOBJECTS = \
	MagickText.o \
	MagickSwirl.o \
	MagickModulate.o \
	MagickMirror.o \
	PluginRegistration.o \

PLUGINNAME = Magick

RESOURCES = \
net.fxarena.openfx.MagickText.png \
net.fxarena.openfx.MagickText.svg \
net.fxarena.openfx.MagickSwirl.png \
net.fxarena.openfx.MagickSwirl.svg \
net.fxarena.openfx.MagickModulate.png \
net.fxarena.openfx.MagickModulate.svg \
net.fxarena.openfx.MagickMirror.png \
net.fxarena.openfx.MagickMirror.svg \

include ../Makefile.master

CXXFLAGS += $(FONTCONFIG_CXXFLAGS) $(MAGICK_CXXFLAGS)
LINKFLAGS += $(FONTCONFIG_CXXFLAGS) $(MAGICK_LINKFLAGS)
