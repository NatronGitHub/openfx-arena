#include "ofxsImageEffect.h"
#include "MagickText.h"
#include "MagickSwirl.h"
#include "MagickMirror.h"
#include "MagickImplode.h"
#include "MagickTile.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getMagickTextPluginID(ids);
            getMagickSwirlPluginID(ids);
            getMagickMirrorPluginID(ids);
            getMagickImplodePluginID(ids);
            getMagickTilePluginID(ids);
        }
    }
}
