#include "ofxsImageEffect.h"
#include "ReadMVG.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getReadMVGPluginID(ids);
        }
    }
}
