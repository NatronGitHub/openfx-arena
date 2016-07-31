PLUGINOBJECTS = \
        Cartoon.o \
        OCLPlugin.o openCLUtilities.o

PLUGINNAME = Cartoon

RESOURCES = \
../OpenCL.png

SRCDIR = ../..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL
VPATH += $(SRCDIR)/OCL/lib

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

