SRCDIR = ../..

PLUGINNAME = CLFilter

PLUGINOBJECTS = \
        $(PLUGINNAME).o \
        OCLPlugin.o

RESOURCES = \
$(SRCDIR)/LICENSE \
$(SRCDIR)/OCL/OpenCL.png \
net.fxarena.opencl.$(PLUGINNAME).png

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

