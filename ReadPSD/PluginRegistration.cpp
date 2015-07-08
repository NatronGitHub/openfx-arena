#include "ofxsImageEffect.h"
#include "ReadPSD.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getReadPSDPluginID(ids);
        }
    }
}
