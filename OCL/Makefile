PLUGINOBJECTS = \
        EdgeCL.o \
        SharpenCL.o \
        RippleCL.o \
        SwirlCL.o \
        TestCL.o \
        openCLUtilities.o \
        ofxsTransform3x3.o ofxsTransformInteractCustom.o ofxsShutter.o

PLUGINNAME = OCL

RESOURCES = \
fr.inria.openfx.EdgeCL.png \
fr.inria.openfx.EdgeCL.svg \
fr.inria.openfx.RippleCL.png \
fr.inria.openfx.RippleCL.svg \
fr.inria.openfx.SwirlCL.png \
fr.inria.openfx.SwirlCL.svg

include ../Makefile.master

VPATH += lib
CXXFLAGS += -Ilib -IOpenCL
LINKFLAGS += -lOpenCL

#CXXFLAGS += -I/opt/AMDAPPSDK-3.0/include
#LINKFLAGS += -L/opt/AMDAPPSDK-3.0/lib/x86_64/sdk
