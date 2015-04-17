PLUGINOBJECTS = \
	MagickText.o \
	PluginRegistration.o \

PLUGINNAME = Magick

RESOURCES = \
net.fxarena.openfx.MagickText.png \
net.fxarena.openfx.MagickText.svg \

include ../Makefile.master

CXXFLAGS += $(OIIO_CXXFLAGS) $(FONTCONFIG_CXXFLAGS) $(MAGICK_CXXFLAGS)
LINKFLAGS += $(OIIO_LINKFLAGS) $(FONTCONFIG_CXXFLAGS) $(MAGICK_LINKFLAGS)
