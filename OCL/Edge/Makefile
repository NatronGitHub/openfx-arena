PLUGINOBJECTS = \
        Edge.o \
        OCLPlugin.o openCLUtilities.o

PLUGINNAME = Edge

RESOURCES = \
../OpenCL.png \
net.fxarena.opencl.Edge.png \
net.fxarena.opencl.Edge.svg

SRCDIR = ../..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL
VPATH += $(SRCDIR)/OCL/lib

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

