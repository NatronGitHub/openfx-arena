PLUGINNAME = BlackmagicRAW

PLUGINOBJECTS = \
    BlackmagicRAWHandler.o \
    BlackmagicRAWPlugin.o \
    BlackmagicRawAPIDispatch.o

PLUGINOBJECTS += \
    GenericReader.o \
    GenericOCIO.o \
    ofxsMultiPlane.o \
    SequenceParsing.o

RESOURCES = \
    net.sf.openfx.BlackmagicRAW.png \
    net.sf.openfx.BlackmagicRAW.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

ifeq ($(OS),Linux)
VPATH += $(SRCDIR)/BlackmagicRAW/sdk/Linux/Include
CXXFLAGS += -I$(SRCDIR)/BlackmagicRAW/sdk/Linux/Include
endif
ifeq ($(OS),Darwin)
VPATH += $(SRCDIR)/BlackmagicRAW/sdk/Mac/Include
CXXFLAGS += -I$(SRCDIR)/BlackmagicRAW/sdk/Mac/Include
endif
ifeq ($(OS:MINGW%=MINGW),MINGW)
VPATH += $(SRCDIR)/BlackmagicRAW/sdk/Win/Include
CXXFLAGS += -I$(SRCDIR)/BlackmagicRAW/sdk/Win/Include
LINKFLAGS += -lole32 -loleaut32
endif
