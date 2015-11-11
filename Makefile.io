CXXFLAGS += -I../OpenFX-IO/IOSupport
VPATH += ../OpenFX-IO/IOSupport ../OpenFX-IO/IOSupport/SequenceParsing

CXXFLAGS += $(shell pkg-config --cflags OpenColorIO)
CXXFLAGS += -DOFX_IO_USING_OCIO
LINKFLAGS += $(shell pkg-config --libs OpenColorIO)
ifeq ($(OS),Linux)
LINKFLAGS += -Wl,-rpath,`pkg-config --variable=libdir OpenColorIO`
endif
