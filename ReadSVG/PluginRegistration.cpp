#include "ofxsImageEffect.h"
#include "ReadSVG.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getReadSVGPluginID(ids);
        }
    }
}
