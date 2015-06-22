#include "ofxsImageEffect.h"
#include "ReadMisc.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getReadMiscPluginID(ids);
        }
    }
}
