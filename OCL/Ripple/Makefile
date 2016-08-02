PLUGINOBJECTS = \
        Ripple.o \
        OCLPlugin.o

PLUGINNAME = Ripple

RESOURCES = \
../OpenCL.png \
net.fxarena.opencl.Ripple.png \
net.fxarena.opencl.Ripple.svg

SRCDIR = ../..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

