PLUGINOBJECTS = \
	MagickText.o \
	MagickSwirl.o \
	MagickMirror.o \
	MagickImplode.o \
	MagickTile.o \
        PluginRegistrationCombined.o \

PLUGINNAME = Arena

RESOURCES = \
../Magick/net.fxarena.openfx.MagickText.png \
../Magick/net.fxarena.openfx.MagickText.svg \
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

include ../Makefile.master

CXXFLAGS += -I../Magick

CXXFLAGS  += $(FONTCONFIG_CXXFLAGS) $(MAGICK_CXXFLAGS)
LINKFLAGS += $(FONTCONFIG_LINKFLAGS) $(MAGICK_LINKFLAGS)
