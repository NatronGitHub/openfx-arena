PLUGINOBJECTS = \
        Bulge.o \
        Cartoon.o \
        Duotone.o \
        Edge.o \
        Looks.o \
        Ripple.o \
        Sharpen.o \
        Swirl.o \
        OCLPlugin.o \
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

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

$(OBJECTPATH)/Bulge.o: Bulge.cpp OCLPlugin.h
$(OBJECTPATH)/Cartoon.o: Cartoon.cpp OCLPlugin.h
$(OBJECTPATH)/Duotone.o: Duotone.cpp OCLPlugin.h
$(OBJECTPATH)/Edge.o: Edge.cpp OCLPlugin.h
$(OBJECTPATH)/Looks.o: Looks.cpp OCLPlugin.h
$(OBJECTPATH)/Ripple.o: Ripple.cpp OCLPlugin.h
$(OBJECTPATH)/Sharpen.o: Sharpen.cpp OCLPlugin.h
$(OBJECTPATH)/Swirl.o: Swirl.cpp OCLPlugin.h

