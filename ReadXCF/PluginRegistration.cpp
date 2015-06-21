#include "ofxsImageEffect.h"
#include "ReadXCF.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getReadXCFPluginID(ids);
        }
    }
}
