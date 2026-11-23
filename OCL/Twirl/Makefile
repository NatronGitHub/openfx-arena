SRCDIR = ../..

PLUGINOBJECTS = \
        Twirl.o \
        OCLPlugin.o \
        ofxsTransformInteractCustom.o

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
