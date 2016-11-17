PLUGINNAME = TextFX

PLUGINOBJECTS = \
    TextFX.o \
    ofxsTransform3x3.o \
    ofxsTransformInteract.o \
    ofxsShutter.o

RESOURCES = \
    net.fxarena.openfx.Text.png \
    net.fxarena.openfx.Text.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master

CXXFLAGS  += \
    -I. \
    $(FCONFIG_CXXFLAGS) \
    $(PANGO_CXXFLAGS) \
    $(GLIB_CXXFLAGS)
LINKFLAGS += \
    $(FCONFIG_LINKFLAGS) \
    $(PANGO_LINKFLAGS) \
    $(GLIB_LINKFLAGS)

