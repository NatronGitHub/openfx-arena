PLUGINNAME = Arena
PLUGINOBJECTS = \
    TextFX.o \
    ReadCDR.o \
    ReadSVG.o \
    ReadKrita.o \
    OpenRaster.o \
    ReadPDF.o \
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
    Text.o \
    lodepng.o \
    MagickPlugin.o \
    ofxsOGLTextRenderer.o \
    ofxsOGLFontData.o \
    ofxsRectangleInteract.o \
    ofxsTransform3x3.o \
    ofxsTransformInteract.o \
    ofxsShutter.o \
    ofxsMultiPlane.o \
    GenericReader.o \
    GenericOCIO.o \
    SequenceParsing.o
RESOURCES = \
    ../share/Arena/Extra.png \
    ../share/Arena/Extra.svg \
    ../share/images/net.fxarena.openfx.Text.png \
    ../share/images/net.fxarena.openfx.Text.svg \
    ../share/images/fr.inria.openfx.ReadCDR.png \
    ../share/images/fr.inria.openfx.ReadCDR.svg \
    ../share/images/net.fxarena.openfx.ReadSVG.png \
    ../share/images/net.fxarena.openfx.ReadSVG.svg \
    ../share/images/fr.inria.openfx.ReadKrita.png \
    ../share/images/fr.inria.openfx.ReadKrita.svg \
    ../share/images/fr.inria.openfx.OpenRaster.png \
    ../share/images/fr.inria.openfx.OpenRaster.svg \
    ../share/images/fr.inria.openfx.ReadPDF.png \
    ../share/images/fr.inria.openfx.ReadPDF.svg \
    ../share/images/net.fxarena.openfx.Tile.png \
    ../share/images/net.fxarena.openfx.Tile.svg \
    ../share/images/net.fxarena.openfx.Reflection.png \
    ../share/images/net.fxarena.openfx.Reflection.svg \
    ../share/images/net.fxarena.openfx.Arc.png \
    ../share/images/net.fxarena.openfx.Arc.svg \
    ../share/images/net.fxarena.openfx.Polar.png \
    ../share/images/net.fxarena.openfx.Polar.svg \
    ../share/images/net.fxarena.openfx.Roll.png \
    ../share/images/net.fxarena.openfx.Roll.svg \
    ../share/images/net.fxarena.openfx.Wave.png \
    ../share/images/net.fxarena.openfx.Wave.svg \
    ../share/images/net.fxarena.openfx.Swirl.png \
    ../share/images/net.fxarena.openfx.Swirl.svg \
    ../share/images/net.fxarena.openfx.Implode.png \
    ../share/images/net.fxarena.openfx.Implode.svg \
    ../share/images/net.fxarena.openfx.Charcoal.png \
    ../share/images/net.fxarena.openfx.Charcoal.svg \
    ../share/images/net.fxarena.openfx.Oilpaint.png \
    ../share/images/net.fxarena.openfx.Oilpaint.svg \
    ../share/images/net.fxarena.openfx.ReadPSD.png \
    ../share/images/net.fxarena.openfx.ReadPSD.svg \
    ../share/images/net.fxarena.openfx.Modulate.png \
    ../share/images/net.fxarena.openfx.Modulate.svg \
    ../share/images/fr.inria.openfx.ReadMisc.png \
    ../share/images/fr.inria.openfx.ReadMisc.svg \
    ../share/images/net.fxarena.openfx.Texture.png \
    ../share/images/net.fxarena.openfx.Texture.svg \
    ../share/images/net.fxarena.openfx.Sketch.png \
    ../share/images/net.fxarena.openfx.Sketch.svg \
    ../share/images/net.fxarena.openfx.Polaroid.png \
    ../share/images/net.fxarena.openfx.Polaroid.svg \
    ../share/images/net.fxarena.openfx.Edges.png \
    ../share/images/net.fxarena.openfx.Edges.svg

SRCDIR = ..
VPATH += $(SRCDIR)/src $(SRCDIR)/common/lodepng $(SRCDIR)/Magick

include $(SRCDIR)/common/Makefile.master
include $(SRCDIR)/common/Makefile.io

CXXFLAGS  += \
    $(FCONFIG_CXXFLAGS) \
    $(PANGO_CXXFLAGS) \
    $(SVG_CXXFLAGS) \
    $(CDR_CXXFLAGS) \
    $(XML_CXXFLAGS) \
    $(ZIP_CXXFLAGS) \
    $(MAGICK_CXXFLAGS) \
    $(LCMS_CXXFLAGS) \
    $(GLIB_CXXFLAGS) \
    $(POPPLER_CXXFLAGS)

CXXFLAGS += -I$(SRCDIR)/src -I$(SRCDIR)/common/lodepng -I$(SRCDIR)/Magick

LINKFLAGS += \
    $(FCONFIG_LINKFLAGS) \
    $(PANGO_LINKFLAGS) \
    $(SVG_LINKFLAGS) \
    $(CDR_LINKFLAGS) \
    $(XML_LINKFLAGS) \
    $(ZIP_LINKFLAGS) \
    $(MAGICK_LINKFLAGS) \
    $(LCMS_LINKFLAGS) \
    $(GLIB_LINKFLAGS) \
    $(POPPLER_LINKFLAGS)

$(OBJECTPATH)/lodepng.o: lodepng.cpp lodepng.h
$(OBJECTPATH)/ReadKrita.o: ReadKrita.cpp lodepng.h
$(OBJECTPATH)/OpenRaster.o: OpenRaster.cpp lodepng.h
$(OBJECTPATH)/MagickPlugin.o: MagickPlugin.cpp MagickPlugin.h
