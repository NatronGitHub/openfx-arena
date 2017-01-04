SRCDIR = ..

PLUGINOBJECTS = \
        Sharpen.o \
        OCLPlugin.o

PLUGINNAME = Sharpen

RESOURCES = \
$(SRCDIR)/LICENSE \
net.fxarena.opencl.Sharpen.png \
net.fxarena.opencl.Sharpen.svg \
net.fxarena.opencl.Sharpen.cl

include $(SRCDIR)/Makefile.master

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)
