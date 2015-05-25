#include "ofxsImageEffect.h"
#include "Text.h"
#include "Distort.h"
#include "Mirror.h"
#include "Texture.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getTextPluginID(ids);
            getDistortPluginID(ids);
            getMirrorPluginID(ids);
            getTexturePluginID(ids);
        }
    }
}
