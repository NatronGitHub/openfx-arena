SRCDIR = ../..

PLUGINOBJECTS = \
        Twirl.o \
        OCLPlugin.o \
        ofxsTransform3x3.o \
        ofxsTransformInteractCustom.o \
        ofxsShutter.o

PLUGINNAME = Twirl

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.Twirl.png \
net.fxarena.opencl.Twirl.svg \
net.fxarena.opencl.Twirl.cl

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)
