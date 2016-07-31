PLUGINOBJECTS = \
        EdgeCL.o \
        OCLPlugin.o \
        openCLUtilities.o \


PLUGINNAME = OCL

RESOURCES = \
OCL.png \
fr.inria.openfx.EdgeCL.png \
fr.inria.openfx.EdgeCL.svg \

include ../Makefile.master

VPATH += lib
CXXFLAGS += -Ilib -IOpenCL

OS := $(shell uname -s)
ifeq ($(OS),Darwin)
LINKFLAGS += -framework OpenCL
else
LINKFLAGS += -lOpenCL
endif

CXXFLAGS += -I/opt/AMDAPPSDK-3.0/include
LINKFLAGS += -L/opt/AMDAPPSDK-3.0/lib/x86_64/sdk

