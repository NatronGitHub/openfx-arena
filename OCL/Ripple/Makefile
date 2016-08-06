SRCDIR = ../..

PLUGINOBJECTS = \
        Ripple.o \
        OCLPlugin.o

PLUGINNAME = Ripple

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.Ripple.png \
net.fxarena.opencl.Ripple.svg \
net.fxarena.opencl.Ripple.cl

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

