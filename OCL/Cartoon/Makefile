PLUGINOBJECTS = \
        Cartoon.o \
        OCLPlugin.o

PLUGINNAME = Cartoon

RESOURCES = \
../OpenCL.png

SRCDIR = ../..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

