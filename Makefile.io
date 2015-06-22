CXXFLAGS += -I../OpenFX-IO/IOSupport
VPATH += ../OpenFX-IO/IOSupport ../OpenFX-IO/IOSupport/SequenceParsing

CXXFLAGS += `pkg-config --cflags OpenColorIO` -DOFX_IO_USING_OCIO
LINKFLAGS += `pkg-config --libs OpenColorIO`
ifeq ($(OS),Linux)
LINKFLAGS += -Wl,-rpath,`pkg-config --variable=libdir OpenColorIO`
endif
