PLUGINNAME = Text

PLUGINOBJECTS = RichText.o TextFX.o

PLUGINOBJECTS += \
    ofxsTransform3x3.o \
    ofxsTransformInteract.o

RESOURCES = \
    net.fxarena.openfx.Text.png \
    net.fxarena.openfx.Text.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master

CXXFLAGS  += $(FCONFIG_CXXFLAGS) $(PANGO_CXXFLAGS)
LINKFLAGS += $(FCONFIG_LINKFLAGS) $(PANGO_LINKFLAGS)

