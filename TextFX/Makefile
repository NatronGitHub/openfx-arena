# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
#

PLUGINOBJECTS = \
        Text.o \
        TextPango.o \
        ofxsOGLTextRenderer.o \
        ofxsOGLFontData.o \
        ofxsRectangleInteract.o \
        PluginRegistrationCombined.o

PLUGINNAME = TextFX

RESOURCES = \
../Text/net.fxarena.openfx.Text.png \
../Text/net.fxarena.openfx.Text.svg \
../TextPango/net.fxarena.openfx.TextPango.png \
../TextPango/net.fxarena.openfx.TextPango.svg

VPATH += \
../Text \
../TextPango

include ../Makefile.master

CXXFLAGS += \
-I../Text \
-I../TextPango

CXXFLAGS  += $(MAGICK_CXXFLAGS)
LINKFLAGS += $(MAGICK_LINKFLAGS)
