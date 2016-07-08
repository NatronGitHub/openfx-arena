PLUGINOBJECTS = \
        EdgeCL.o \
        openCLUtilities.o

PLUGINNAME = OCL

RESOURCES = \
fr.inria.openfx.EdgeCL.png \
fr.inria.openfx.EdgeCL.svg

include ../Makefile.master

VPATH += lib
CXXFLAGS += -Ilib -IOpenCL
LINKFLAGS += -lOpenCL

