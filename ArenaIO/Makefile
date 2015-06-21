PLUGINOBJECTS = \
    ArenaIO.o \
    PluginRegistration.o \
    GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ArenaIO

RESOURCES = net.fxarena.openfx.ArenaIO.png net.fxarena.openfx.ArenaIO.svg

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
