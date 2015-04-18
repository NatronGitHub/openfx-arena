PLUGINOBJECTS = \
	MagickText.o \
	MagickSwirl.o \
	PluginRegistration.o \

PLUGINNAME = Magick

RESOURCES = \
net.fxarena.openfx.MagickText.png \
net.fxarena.openfx.MagickText.svg \
net.fxarena.openfx.MagickSwirl.png \
net.fxarena.openfx.MagickSwirl.svg \

include ../Makefile.master

CXXFLAGS += $(FONTCONFIG_CXXFLAGS) $(MAGICK_CXXFLAGS)
LINKFLAGS += $(FONTCONFIG_CXXFLAGS) $(MAGICK_LINKFLAGS)
