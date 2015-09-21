# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
#

PLUGINOBJECTS = \
        ReadPSD.o \
        ReadSVG.o \
        PluginRegistrationCombined.o \
        GenericReader.o GenericOCIO.o SequenceParsing.o

PLUGINNAME = ExtraIO

RESOURCES = \
../ReadPSD/net.fxarena.openfx.ReadPSD.png \
../ReadPSD/net.fxarena.openfx.ReadPSD.svg \
../ReadSVG/net.fxarena.openfx.ReadSVG.png \
../ReadSVG/net.fxarena.openfx.ReadSVG.svg

VPATH += \
../ReadPSD \
../ReadSVG

include ../Makefile.master
include ../Makefile.io

CXXFLAGS += \
-I../ReadPSD \
-I../ReadSVG

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
