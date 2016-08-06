SRCDIR = ../..

PLUGINOBJECTS = \
        Cartoon.o \
        OCLPlugin.o

PLUGINNAME = Cartoon

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.Cartoon.png \
net.fxarena.opencl.Cartoon.svg \
net.fxarena.opencl.Cartoon.cl

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

