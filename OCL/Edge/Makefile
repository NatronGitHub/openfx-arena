PLUGINOBJECTS = \
        Edge.o \
        OCLPlugin.o

PLUGINNAME = Edge

RESOURCES = \
../OpenCL.png \
net.fxarena.opencl.Edge.png \
net.fxarena.opencl.Edge.svg

SRCDIR = ../..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

