SRCDIR = ../..

PLUGINNAME= Bloom

PLUGINOBJECTS = \
        $(PLUGINNAME).o \
        OCLPlugin.o

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.$(PLUGINNAME).png \
net.fxarena.opencl.$(PLUGINNAME).svg \
net.fxarena.opencl.$(PLUGINNAME).cl \
$(SRCDIR)/OCL/OpenCL.png

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

