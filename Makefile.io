CXXFLAGS += -I$(SRCDIR)/OpenFX-IO/IOSupport -I$(SRCDIR)/SupportExt/glad
VPATH += $(SRCDIR)/OpenFX-IO/IOSupport $(SRCDIR)/OpenFX-IO/IOSupport/SequenceParsing $(SRCDIR)/SupportExt/glad

CXXFLAGS += $(shell pkg-config --cflags OpenColorIO)
CXXFLAGS += -DOFX_IO_USING_OCIO
LINKFLAGS += $(shell pkg-config --libs OpenColorIO)
ifeq ($(OS),Linux)
LINKFLAGS += -Wl,-rpath,`pkg-config --variable=libdir OpenColorIO`
endif
