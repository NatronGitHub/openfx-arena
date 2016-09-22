PLUGINNAME = HaldCLUT
PLUGINOBJECTS = \
        $(PLUGINNAME).o \
        MagickPlugin.o
SRCDIR = ../..
include $(SRCDIR)/Magick/Makefile.Magick

RESOURCES += \
net.fxarena.openfx.$(PLUGINNAME).xml

CXXFLAGS  += $(CURL_CXXFLAGS) $(XML_CXXFLAGS)
LINKFLAGS += $(CURL_LINKFLAGS) $(XML_LINKFLAGS)
