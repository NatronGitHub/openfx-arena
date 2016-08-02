PLUGINOBJECTS = \
        Bulge.o \
        OCLPlugin.o \
        ofxsTransform3x3.o ofxsTransformInteractCustom.o ofxsShutter.o

PLUGINNAME = Bulge

RESOURCES = \
../OpenCL.png

SRCDIR = ../..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

