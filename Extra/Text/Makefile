PLUGINNAME = TextFX

PLUGINOBJECTS = \
TextFX.o \
ofxsTransform3x3.o \
ofxsTransformInteract.o \
ofxsShutter.o

RESOURCES = \
net.fxarena.openfx.Text.png \
net.fxarena.openfx.Text.svg 

SRCDIR = ../..
include $(SRCDIR)/Makefile.master

CXXFLAGS  += \
$(FCONFIG_CXXFLAGS) \
$(PANGO_CXXFLAGS) 

LINKFLAGS += \
$(FCONFIG_LINKFLAGS) \
$(PANGO_LINKFLAGS)

# disable fontconfig (win-only)
ifeq ($(NOFC),1)
CXXFLAGS += -DNOFC
LDFLAGS += -gdi32
endif
