SRCDIR = ../..

PLUGINNAME = Bulge

PLUGINOBJECTS = \
        $(PLUGINNAME).o \
        OCLPlugin.o \
        ofxsTransform3x3.o \
        ofxsTransformInteractCustom.o \
        ofxsShutter.o

RESOURCES = \
$(SRCDIR)/LICENSE \
$(SRCDIR)/OCL/OpenCL.png \
net.fxarena.opencl.$(PLUGINNAME).png \
net.fxarena.opencl.$(PLUGINNAME).svg \
net.fxarena.opencl.$(PLUGINNAME).cl

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

