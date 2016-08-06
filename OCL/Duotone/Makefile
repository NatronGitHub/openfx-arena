SRCDIR = ../..

PLUGINOBJECTS = \
        Duotone.o \
        OCLPlugin.o

PLUGINNAME = Duotone

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.Duotone.png \
net.fxarena.opencl.Duotone.svg \
net.fxarena.opencl.Duotone.cl

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

