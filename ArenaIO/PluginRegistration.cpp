#include "ofxsImageEffect.h"
#include "ArenaIO.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getArenaIOPluginID(ids);
        }
    }
}
