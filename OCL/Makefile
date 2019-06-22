PLUGINNAME = ArenaCL

PLUGINOBJECTS = \
    Bulge.o \
    Duotone.o \
    Ripple.o \
    Twirl.o \
    Vibrance.o \
    Bloom.o \
    OCLPlugin.o \
    ofxsTransform3x3.o \
    ofxsTransformInteractCustom.o \
    ofxsShutter.o

RESOURCES = \
    OpenCL.png \
    Ripple/net.fxarena.opencl.Ripple.png \
    Ripple/net.fxarena.opencl.Ripple.svg \
    Ripple/net.fxarena.opencl.Ripple.cl \
    Twirl/net.fxarena.opencl.Twirl.png \
    Twirl/net.fxarena.opencl.Twirl.svg \
    Twirl/net.fxarena.opencl.Twirl.cl \
    Bulge/net.fxarena.opencl.Bulge.png \
    Bulge/net.fxarena.opencl.Bulge.svg \
    Bulge/net.fxarena.opencl.Bulge.cl \
    Duotone/net.fxarena.opencl.Duotone.png \
    Duotone/net.fxarena.opencl.Duotone.svg \
    Duotone/net.fxarena.opencl.Duotone.cl \
    Vibrance/net.fxarena.opencl.Vibrance.png \
    Vibrance/net.fxarena.opencl.Vibrance.svg \
    Vibrance/net.fxarena.opencl.Vibrance.cl \
    Bloom/net.fxarena.opencl.Bloom.png \
    Bloom/net.fxarena.opencl.Bloom.svg \
    Bloom/net.fxarena.opencl.Bloom.cl

VPATH += \
    Bulge \
    Duotone \
    Ripple \
    Twirl \
    Vibrance \
    Bloom

CXXFLAGS += \
    -IBulge \
    -IDuotone \
    -IRipple \
    -ITwirl \
    -IVibrance \
    -IBloom

SRCDIR = ..
include $(SRCDIR)/Makefile.master

CXXFLAGS += $(OPENCL_CXXFLAGS)
LINKFLAGS += $(OPENCL_LINKFLAGS)

$(OBJECTPATH)/Bulge.o: Bulge.cpp OCLPlugin.h
$(OBJECTPATH)/Duotone.o: Duotone.cpp OCLPlugin.h
$(OBJECTPATH)/Ripple.o: Ripple.cpp OCLPlugin.h
$(OBJECTPATH)/Twirl.o: Twirl.cpp OCLPlugin.h
$(OBJECTPATH)/Vibrance.o: Vibrance.cpp OCLPlugin.h
$(OBJECTPATH)/Bloom.o: Bloom.cpp OCLPlugin.h
$(OBJECTPATH)/CLFilter.o: CLFilter.cpp OCLPlugin.h

