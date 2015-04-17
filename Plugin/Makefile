PLUGINOBJECTS = \
	MagickText.o \
	ofxsOGLTextRenderer.o \
	ofxsOGLFontData.o \
	ofxsGenerator.o \
	ofxsRectangleInteract.o \
	PluginRegistrationCombined.o

PLUGINNAME = Test

# ls */*png */*svg|sed -e 's!^!../!' -e 's!$! \\!'|sort
RESOURCES = \
../Magick/net.fxarena.openfx.MagickText.png \
../Magick/net.fxarena.openfx.MagickText.svg \

VPATH += \
../Magick \

include ../Makefile.master

CXXFLAGS += -I../Magick

CXXFLAGS  += $(OIIO_CXXFLAGS) $(FONTCONFIG_CXXFLAGS)
LINKFLAGS += $(OIIO_LINKFLAGS)  $(FONTCONFIG_LINKFLAGS)
