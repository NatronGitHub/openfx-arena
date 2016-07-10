PLUGINOBJECTS = \
        EdgeCL.o \
        SharpenCL.o \
        openCLUtilities.o
        #openCLGLUtilities.o

PLUGINNAME = OCL

RESOURCES = \
fr.inria.openfx.EdgeCL.png \
fr.inria.openfx.EdgeCL.svg

include ../Makefile.master

VPATH += lib
CXXFLAGS += -Ilib -IOpenCL
LINKFLAGS += -lOpenCL

#CXXFLAGS += -I/opt/AMDAPPSDK-3.0/include
#LINKFLAGS += -L/opt/AMDAPPSDK-3.0/lib/x86_64/sdk
