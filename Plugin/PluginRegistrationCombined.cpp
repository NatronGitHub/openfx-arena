#include "ofxsImageEffect.h"
#include "MagickText.h"
#include "MagickSwirl.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getMagickTextPluginID(ids);
            getMagickSwirlPluginID(ids);
        }
    }
}
