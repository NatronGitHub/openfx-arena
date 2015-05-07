#include "ofxsImageEffect.h"
#include "Text.h"
#include "Swirl.h"
#include "Mirror.h"
#include "Implode.h"
#include "Tile.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getTextPluginID(ids);
            getSwirlPluginID(ids);
            getMirrorPluginID(ids);
            getImplodePluginID(ids);
            getTilePluginID(ids);
        }
    }
}
