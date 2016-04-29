PLUGINOBJECTS = \
        TextFX.o

PLUGINNAME = Extra

RESOURCES = \
../TextFX/fr.inria.openfx.TextFX.png \
../TextFX/fr.inria.openfx.TextFX.svg

VPATH += \
../TextFX

include ../Makefile.master

CXXFLAGS += \
-I../TextFX

CXXFLAGS  += $(FCONFIG_CXXFLAGS) $(PANGO_CXXFLAGS)
LINKFLAGS += $(FCONFIG_LINKFLAGS) $(PANGO_LINKFLAGS)

