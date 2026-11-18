CXXFLAGS += \
    -I$(SRCDIR)/OpenFX-IO/IOSupport \
    -I$(SRCDIR)/SupportExt/glad
VPATH += \
    $(SRCDIR)/OpenFX-IO/IOSupport \
    $(SRCDIR)/OpenFX-IO/IOSupport/SequenceParsing \
    $(SRCDIR)/SupportExt/glad

CXXFLAGS += $(shell pkg-config --cflags OpenColorIO)
CXXFLAGS += -DOFX_IO_USING_OCIO
# no -rpath into OpenColorIO's libdir: a bundle's RUNPATH only has $ORIGIN-relative entries
# (openfx's linuxCheckBundle.sh); the installers bundle OpenColorIO in <bundle>/Libraries
LINKFLAGS += $(shell pkg-config --libs OpenColorIO)
