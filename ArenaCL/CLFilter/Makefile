SRCDIR = ../..

PLUGINOBJECTS = \
        CLFilter.o \
        OCLPlugin.o

PLUGINNAME = CLFilter

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.CLFilter.png

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

