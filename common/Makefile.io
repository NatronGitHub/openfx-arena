CXXFLAGS += \
    -I$(SRCDIR)/common/OpenFX-IO/IOSupport \
    -I$(SRCDIR)/common/SupportExt/glad
VPATH += \
    $(SRCDIR)/common/OpenFX-IO/IOSupport \
    $(SRCDIR)/common/OpenFX-IO/IOSupport/SequenceParsing \
    $(SRCDIR)/common/SupportExt/glad

CXXFLAGS += $(shell pkg-config --cflags OpenColorIO)
CXXFLAGS += -DOFX_IO_USING_OCIO
LINKFLAGS += $(shell pkg-config --libs OpenColorIO)
ifeq ($(OS),Linux)
LINKFLAGS += -Wl,-rpath,`pkg-config --variable=libdir OpenColorIO`
endif
