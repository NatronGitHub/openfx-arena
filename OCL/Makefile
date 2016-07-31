PLUGINOBJECTS = \
        Bulge.o \
        Cartoon.o \
        Duotone.o \
        Edge.o \
        Looks.o \
        Ripple.o \
        Sharpen.o \
        Swirl.o \
        OCLPlugin.o openCLUtilities.o \
        ofxsTransform3x3.o ofxsTransformInteractCustom.o ofxsShutter.o

PLUGINNAME = OCL

RESOURCES = \
OpenCL.png \
Edge/net.fxarena.opencl.Edge.png \
Edge/net.fxarena.opencl.Edge.svg \
Ripple/net.fxarena.opencl.Ripple.png \
Ripple/net.fxarena.opencl.Ripple.svg \
Swirl/net.fxarena.opencl.Swirl.png \
Swirl/net.fxarena.opencl.Swirl.svg

VPATH += \
Bulge \
Cartoon \
Duotone \
Edge \
Looks \
Ripple \
Sharpen \
Swirl

CXXFLAGS += \
-I. \
-IBulge \
-ICartoon \
-IDuotone \
-IEdge \
-ILooks \
-IRipple \
-ISharpen \
-ISwirl

SRCDIR = ..

include $(SRCDIR)/Makefile.master

VPATH += $(SRCDIR)/OCL
VPATH += $(SRCDIR)/OCL/lib
CXXFLAGS += -I$(SRCDIR)/OCL
CXXFLAGS += -I$(SRCDIR)/OCL/lib
LINKFLAGS += $(OPENCL_LINKFLAGS)

#CXXFLAGS += -I/opt/AMDAPPSDK-3.0/include
#LINKFLAGS += -L/opt/AMDAPPSDK-3.0/lib/x86_64/sdk
