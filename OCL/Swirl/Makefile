PLUGINOBJECTS = \
        Swirl.o \
        OCLPlugin.o openCLUtilities.o \
        ofxsTransform3x3.o ofxsTransformInteractCustom.o ofxsShutter.o

PLUGINNAME = Swirl

RESOURCES = \
../OpenCL.png

SRCDIR = ../..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL
VPATH += $(SRCDIR)/OCL/lib

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

