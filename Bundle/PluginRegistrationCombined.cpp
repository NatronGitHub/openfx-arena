#include "ofxsImageEffect.h"
#include "Mirror.h"
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
#include "Emboss.h"
#include "Charcoal.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getMirrorPluginID(ids);
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
            getEmbossPluginID(ids);
            getCharcoalPluginID(ids);
        }
    }
}
