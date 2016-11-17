PLUGINNAME = Magick

ifneq ($(LEGACYIM),1)
PLUGINOBJECTS += \
    Sketch.o \
    Polaroid.o \
    Edges.o
endif

PLUGINOBJECTS = \
    Texture.o \
    Tile.o \
    Reflection.o \
    Arc.o \
    Polar.o \
    Roll.o \
    Wave.o \
    Swirl.o \
    Implode.o \
    Charcoal.o \
    Oilpaint.o \
    ReadPSD.o \
    Modulate.o \
    ReadMisc.o \
    Text.o \
    HaldCLUT.o \
    MagickPlugin.o \
    ofxsOGLTextRenderer.o \
    ofxsOGLFontData.o \
    ofxsRectangleInteract.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o

RESOURCES = \
    Extra.png \
    Extra.svg \
    net.fxarena.openfx.Tile.png \
    net.fxarena.openfx.Tile.svg \
    net.fxarena.openfx.Reflection.png \
    net.fxarena.openfx.Reflection.svg \
    Arc/net.fxarena.openfx.Arc.png \
    Arc/net.fxarena.openfx.Arc.svg \
    net.fxarena.openfx.Polar.png \
    net.fxarena.openfx.Polar.svg \
    Roll/net.fxarena.openfx.Roll.png \
    Roll/net.fxarena.openfx.Roll.svg \
    Wave/net.fxarena.openfx.Wave.png \
    Wave/net.fxarena.openfx.Wave.svg \
    Swirl/net.fxarena.openfx.Swirl.png \
    Swirl/net.fxarena.openfx.Swirl.svg \
    net.fxarena.openfx.Implode.png \
    net.fxarena.openfx.Implode.svg \
    net.fxarena.openfx.Charcoal.png \
    net.fxarena.openfx.Charcoal.svg \
    net.fxarena.openfx.Oilpaint.png \
    net.fxarena.openfx.Oilpaint.svg \
    net.fxarena.openfx.ReadPSD.png \
    net.fxarena.openfx.ReadPSD.svg \
    net.fxarena.openfx.Modulate.png \
    net.fxarena.openfx.Modulate.svg \
    fr.inria.openfx.ReadMisc.png \
    fr.inria.openfx.ReadMisc.svg \
    net.fxarena.openfx.Text.png \
    net.fxarena.openfx.Text.svg \
    net.fxarena.openfx.Texture.png \
    net.fxarena.openfx.Texture.svg \
    HaldCLUT/net.fxarena.openfx.HaldCLUT.png \
    HaldCLUT/net.fxarena.openfx.HaldCLUT.svg \
    HaldCLUT/net.fxarena.openfx.HaldCLUT.xml

ifneq ($(LEGACYIM),1)
RESOURCES += \
    net.fxarena.openfx.Sketch.png \
    net.fxarena.openfx.Sketch.svg \
    net.fxarena.openfx.Polaroid.png \
    net.fxarena.openfx.Polaroid.svg \
    net.fxarena.openfx.Edges.png \
    net.fxarena.openfx.Edges.svg
endif

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

VPATH += \
    $(SRCDIR)/Magick/HaldCLUT \
    $(SRCDIR)/Magick/Swirl \
    $(SRCDIR)/Magick/Roll \
    $(SRCDIR)/Magick/Wave \
    $(SRCDIR)/Magick/Arc

CXXFLAGS += \
    -I.

CXXFLAGS  += \
    $(MAGICK_CXXFLAGS) \
    $(LCMS_CXXFLAGS) \
    $(CURL_CXXFLAGS) \
    $(XML_CXXFLAGS)
LINKFLAGS += \
    $(MAGICK_LINKFLAGS) \
    $(LCMS_LINKFLAGS) \
    $(CURL_LINKFLAGS) \
    $(XML_LINKFLAGS)

ifeq ($(IM),7)
CXXFLAGS += -DIM7
endif

ifeq ($(LEGACYIM),1)
CXXFLAGS += -DLEGACYIM -DNOMAGICKSEED
endif

ifeq ($(IM),68)
CXXFLAGS += -DNOMAGICKSEED
endif
