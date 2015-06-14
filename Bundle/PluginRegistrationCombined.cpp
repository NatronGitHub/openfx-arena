#include "ofxsImageEffect.h"
#include "Distort.h"
#include "Mirror.h"
#include "Texture.h"
#include "Tile.h"
#include "Reflection.h"
#include "Text.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getDistortPluginID(ids);
            getMirrorPluginID(ids);
            getTexturePluginID(ids);
            getTilePluginID(ids);
            getReflectionPluginID(ids);
	    getTextPluginID(ids);
        }
    }
}
