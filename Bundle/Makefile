# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
#

PLUGINOBJECTS = \
        Texture.o \
        Tile.o \
        Reflection.o \
        Text.o \
        Arc.o \
        Polar.o \
        Roll.o \
        Wave.o \
        Swirl.o \
        Implode.o \
        Charcoal.o \
        Oilpaint.o \
        TextPango.o \
        ReadPSD.o \
        ReadSVG.o \
        Sketch.o \
        Polaroid.o \
        DaveHill.o \
        Edges.o \
        ofxsOGLTextRenderer.o \
        ofxsOGLFontData.o \
        ofxsRectangleInteract.o \
        PluginRegistrationCombined.o \
        GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = Arena

RESOURCES = \
../Texture/net.fxarena.openfx.Texture.png \
../Texture/net.fxarena.openfx.Texture.svg \
../Tile/net.fxarena.openfx.Tile.png \
../Tile/net.fxarena.openfx.Tile.svg \
../Reflection/net.fxarena.openfx.Reflection.png \
../Reflection/net.fxarena.openfx.Reflection.svg \
../Text/net.fxarena.openfx.Text.png \
../Text/net.fxarena.openfx.Text.svg \
../Arc/net.fxarena.openfx.Arc.png \
../Arc/net.fxarena.openfx.Arc.svg \
../Polar/net.fxarena.openfx.Polar.png \
../Polar/net.fxarena.openfx.Polar.svg \
../Roll/net.fxarena.openfx.Roll.png \
../Roll/net.fxarena.openfx.Roll.svg \
../Wave/net.fxarena.openfx.Wave.png \
../Wave/net.fxarena.openfx.Wave.svg \
../Swirl/net.fxarena.openfx.Swirl.png \
../Swirl/net.fxarena.openfx.Swirl.svg \
../Implode/net.fxarena.openfx.Implode.png \
../Implode/net.fxarena.openfx.Implode.svg \
../Charcoal/net.fxarena.openfx.Charcoal.png \
../Charcoal/net.fxarena.openfx.Charcoal.svg \
../Oilpaint/net.fxarena.openfx.Oilpaint.png \
../Oilpaint/net.fxarena.openfx.Oilpaint.svg \
../TextPango/net.fxarena.openfx.TextPango.png \
../TextPango/net.fxarena.openfx.TextPango.svg \
../ReadPSD/net.fxarena.openfx.ReadPSD.png \
../ReadPSD/net.fxarena.openfx.ReadPSD.svg \
../ReadSVG/net.fxarena.openfx.ReadSVG.png \
../ReadSVG/net.fxarena.openfx.ReadSVG.svg \
../Sketch/net.fxarena.openfx.Sketch.png \
../Sketch/net.fxarena.openfx.Sketch.svg \
../Polaroid/net.fxarena.openfx.Polaroid.png \
../Polaroid/net.fxarena.openfx.Polaroid.svg \
../DaveHill/net.fxarena.openfx.DaveHill.png \
../DaveHill/net.fxarena.openfx.DaveHill.svg \
../Edges/net.fxarena.openfx.Edges.png \
../Edges/net.fxarena.openfx.Edges.svg \


VPATH += \
../Texture \
../Tile \
../Reflection \
../Text \
../Arc \
../Polar \
../Roll \
../Wave \
../Swirl \
../Implode \
../Charcoal \
../Oilpaint \
../TextPango \
../ReadPSD \
../ReadSVG \
../Sketch \
../Polaroid \
../DaveHill \
../Edges \

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += \
-I../Texture \
-I../Tile \
-I../Reflection \
-I../Text \
-I../Arc \
-I../Polar \
-I../Roll \
-I../Wave \
-I../Swirl \
-I../Implode \
-I../Charcoal \
-I../Oilpaint \
-I../TextPango \
-I../ReadPSD \
-I../ReadSVG \
-I../Sketch \
-I../Polaroid \
-I../DaveHill \
-I../Edges \

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
