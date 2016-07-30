PLUGINOBJECTS = \
        Edge.o \
        OCLPlugin.o openCLUtilities.o

PLUGINNAME = Edge

RESOURCES = \
../OpenCL.png \
fr.inria.openfx.Edge.png \
fr.inria.openfx.Edge.svg

SRCDIR = ../..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL
VPATH += $(SRCDIR)/OCL/lib
CXXFLAGS += -I$(SRCDIR)/OCL
CXXFLAGS += -I$(SRCDIR)/OCL/lib
LINKFLAGS += $(OPENCL_LINKFLAGS)

#CXXFLAGS += -I/opt/AMDAPPSDK-3.0/include
#LINKFLAGS += -L/opt/AMDAPPSDK-3.0/lib/x86_64/sdk
