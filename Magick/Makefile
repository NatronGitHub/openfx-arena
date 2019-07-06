PLUGINNAME = Magick

PLUGINOBJECTS = \
    Sketch.o \
    Polaroid.o \
    Edges.o \
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
    Text.o

PLUGINOBJECTS += \
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
    net.fxarena.openfx.Arc.png \
    net.fxarena.openfx.Arc.svg \
    net.fxarena.openfx.Polar.png \
    net.fxarena.openfx.Polar.svg \
    Implode/net.fxarena.openfx.Implode.png \
    Implode/net.fxarena.openfx.Implode.svg \
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
    net.fxarena.openfx.Sketch.png \
    net.fxarena.openfx.Sketch.svg \
    net.fxarena.openfx.Polaroid.png \
    net.fxarena.openfx.Polaroid.svg \
    net.fxarena.openfx.Edges.png \
    net.fxarena.openfx.Edges.svg \
    Roll/net.fxarena.openfx.Roll.png \
    Roll/net.fxarena.openfx.Roll.svg \
    Wave/net.fxarena.openfx.Wave.png \
    Wave/net.fxarena.openfx.Wave.svg \
    Swirl/net.fxarena.openfx.Swirl.png \
    Swirl/net.fxarena.openfx.Swirl.svg

SRCDIR = ..
include $(SRCDIR)/Makefile.master
include $(SRCDIR)/Makefile.io

VPATH += \
    $(SRCDIR)/Magick/Swirl \
    $(SRCDIR)/Magick/Roll \
    $(SRCDIR)/Magick/Wave \
    $(SRCDIR)/Magick/Implode

CXXFLAGS += \
    -I.

CXXFLAGS  += \
    $(MAGICK_CXXFLAGS) \
    $(LCMS_CXXFLAGS)
LINKFLAGS += \
    $(MAGICK_LINKFLAGS) \
    $(LCMS_LINKFLAGS)

ifeq ($(LEGACYIM),1)
CXXFLAGS += -DLEGACYIM -DNOMAGICKSEED
endif

ifeq ($(IM),68)
CXXFLAGS += -DNOMAGICKSEED
endif
