PLUGINOBJECTS = \
    ReadXCF.o \
    PluginRegistration.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ReadXCF

RESOURCES = net.fxarena.openfx.ReadXCF.png net.fxarena.openfx.ReadXCF.svg

include ../Makefile.master

CXXFLAGS += -I../OpenFX-IO/IOSupport
VPATH += ../OpenFX-IO/IOSupport ../OpenFX-IO/IOSupport/SequenceParsing

CXXFLAGS += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)

CXXFLAGS += `pkg-config --cflags OpenColorIO` -DOFX_IO_USING_OCIO
LINKFLAGS += `pkg-config --libs OpenColorIO`
ifeq ($(OS),Linux)
LINKFLAGS += -Wl,-rpath,`pkg-config --variable=libdir OpenColorIO`
endif
