SRCDIR = ../..

PLUGINOBJECTS = \
        Bulge.o \
        OCLPlugin.o \
        ofxsTransform3x3.o \
        ofxsTransformInteractCustom.o \
        ofxsShutter.o

PLUGINNAME = Bulge

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.Bulge.png \
net.fxarena.opencl.Bulge.svg \
net.fxarena.opencl.Bulge.cl

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

