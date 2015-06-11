#include "ofxsImageEffect.h"
#include "Mirror.h"
#include "Texture.h"
#include "Tile.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getMirrorPluginID(ids);
            getTexturePluginID(ids);
            getTilePluginID(ids);
        }
    }
}
