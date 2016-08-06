SRCDIR = ..

PLUGINOBJECTS = \
        Bulge.o \
        Cartoon.o \
        Duotone.o \
        Edge.o \
        Ripple.o \
        Sharpen.o \
        Twirl.o \
        OCLPlugin.o \
        ofxsTransform3x3.o \
        ofxsTransformInteractCustom.o \
        ofxsShutter.o

PLUGINNAME = OCL

RESOURCES = \
OpenCL.png \
Edge/net.fxarena.opencl.Edge.png \
Edge/net.fxarena.opencl.Edge.svg \
Edge/net.fxarena.opencl.Edge.cl \
Ripple/net.fxarena.opencl.Ripple.png \
Ripple/net.fxarena.opencl.Ripple.svg \
Ripple/net.fxarena.opencl.Ripple.cl \
Twirl/net.fxarena.opencl.Twirl.png \
Twirl/net.fxarena.opencl.Twirl.svg \
Twirl/net.fxarena.opencl.Twirl.cl \
Bulge/net.fxarena.opencl.Bulge.png \
Bulge/net.fxarena.opencl.Bulge.svg \
Bulge/net.fxarena.opencl.Bulge.cl \
Sharpen/net.fxarena.opencl.Sharpen.png \
Sharpen/net.fxarena.opencl.Sharpen.svg \
Sharpen/net.fxarena.opencl.Sharpen.cl \
Cartoon/net.fxarena.opencl.Cartoon.png \
Cartoon/net.fxarena.opencl.Cartoon.svg \
Cartoon/net.fxarena.opencl.Cartoon.cl \
Duotone/net.fxarena.opencl.Duotone.png \
Duotone/net.fxarena.opencl.Duotone.svg \
Duotone/net.fxarena.opencl.Duotone.cl

VPATH += \
Bulge \
Cartoon \
Duotone \
Edge \
Ripple \
Sharpen \
Twirl

CXXFLAGS += \
-IBulge \
-ICartoon \
-IDuotone \
-IEdge \
-IRipple \
-ISharpen \
-ITwirl

include $(SRCDIR)/Makefile.master

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

$(OBJECTPATH)/Bulge.o: Bulge.cpp OCLPlugin.h
$(OBJECTPATH)/Cartoon.o: Cartoon.cpp OCLPlugin.h
$(OBJECTPATH)/Duotone.o: Duotone.cpp OCLPlugin.h
$(OBJECTPATH)/Edge.o: Edge.cpp OCLPlugin.h
$(OBJECTPATH)/Ripple.o: Ripple.cpp OCLPlugin.h
$(OBJECTPATH)/Sharpen.o: Sharpen.cpp OCLPlugin.h
$(OBJECTPATH)/Twirl.o: Twirl.cpp OCLPlugin.h

