SRCDIR = ../..

PLUGINOBJECTS = \
        Edge.o \
        OCLPlugin.o

PLUGINNAME = Edge

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.Edge.png \
net.fxarena.opencl.Edge.svg \
net.fxarena.opencl.Edge.cl

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

