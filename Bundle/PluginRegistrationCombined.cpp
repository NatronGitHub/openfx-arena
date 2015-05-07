#include "ofxsImageEffect.h"
#include "Text.h"
#include "Swirl.h"
#include "MagickMirror.h"
#include "Implode.h"
#include "MagickTile.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getTextPluginID(ids);
            getSwirlPluginID(ids);
            getMagickMirrorPluginID(ids);
            getImplodePluginID(ids);
            getMagickTilePluginID(ids);
        }
    }
}
