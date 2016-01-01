#include "ofxsImageEffect.h"
#include "ReadORA.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getReadORAPluginID(ids);
        }
    }
}
