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
        Arc.o \
        Polar.o \
        Roll.o \
        Wave.o \
        Swirl.o \
        Implode.o \
        Charcoal.o \
        Oilpaint.o \
        Sketch.o \
        Polaroid.o \
        Edges.o \
        Modulate.o \
        PluginRegistrationCombined.o

PLUGINNAME = ExtraFX

RESOURCES = \
../Bundle/Arena.png \
../Bundle/Arena.svg \
../Texture/net.fxarena.openfx.Texture.png \
../Texture/net.fxarena.openfx.Texture.svg \
../Tile/net.fxarena.openfx.Tile.png \
../Tile/net.fxarena.openfx.Tile.svg \
../Reflection/net.fxarena.openfx.Reflection.png \
../Reflection/net.fxarena.openfx.Reflection.svg \
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
../Sketch/net.fxarena.openfx.Sketch.png \
../Sketch/net.fxarena.openfx.Sketch.svg \
../Polaroid/net.fxarena.openfx.Polaroid.png \
../Polaroid/net.fxarena.openfx.Polaroid.svg \
../Edges/net.fxarena.openfx.Edges.png \
../Edges/net.fxarena.openfx.Edges.svg \
../Modulate/net.fxarena.openfx.Modulate.png \
../Modulate/net.fxarena.openfx.Modulate.svg

VPATH += \
../Texture \
../Tile \
../Reflection \
../Arc \
../Polar \
../Roll \
../Wave \
../Swirl \
../Implode \
../Charcoal \
../Oilpaint \
../Sketch \
../Polaroid \
../Edges \
../Modulate

include ../Makefile.master

CXXFLAGS += \
-I../Texture \
-I../Tile \
-I../Reflection \
-I../Arc \
-I../Polar \
-I../Roll \
-I../Wave \
-I../Swirl \
-I../Implode \
-I../Charcoal \
-I../Oilpaint \
-I../Sketch \
-I../Polaroid \
-I../Edges \
-I../Modulate

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
