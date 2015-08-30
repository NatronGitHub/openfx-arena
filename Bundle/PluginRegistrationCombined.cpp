/*
# Copyright (c) 2015, FxArena DA <mail@fxarena.net>
# All rights reserved.
#
# OpenFX-Arena is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 2. You should have received a copy of the GNU General Public License version 2 along with OpenFX-Arena. If not, see http://www.gnu.org/licenses/.
# OpenFX-Arena is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Need custom licensing terms or conditions? Commercial license for proprietary software? Contact us.
*/

#include "ofxsImageEffect.h"
#include "Texture.h"
#include "Tile.h"
#include "Reflection.h"
#include "Text.h"
#include "Arc.h"
#include "Polar.h"
#include "Roll.h"
#include "Wave.h"
#include "Swirl.h"
#include "Implode.h"
#include "Charcoal.h"
#include "Oilpaint.h"
#include "TextPango.h"
#include "ReadPSD.h"
#include "ReadSVG.h"
#include "Sketch.h"
#include "Polaroid.h"
#include "DaveHill.h"
#include "Edges.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getTexturePluginID(ids);
            getTilePluginID(ids);
            getReflectionPluginID(ids);
            getTextPluginID(ids);
            getArcPluginID(ids);
            getPolarPluginID(ids);
            getRollPluginID(ids);
            getWavePluginID(ids);
            getSwirlPluginID(ids);
            getImplodePluginID(ids);
            getCharcoalPluginID(ids);
            getOilpaintPluginID(ids);
            getTextPangoPluginID(ids);
            getReadPSDPluginID(ids);
            getReadSVGPluginID(ids);
            getSketchPluginID(ids);
            getPolaroidPluginID(ids);
            getDaveHillPluginID(ids);
            getEdgesPluginID(ids);
        }
    }
}
