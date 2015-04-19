#include "ofxsImageEffect.h"
#include "MagickText.h"
#include "MagickSwirl.h"
#include "MagickModulate.h"
#include "MagickMirror.h"
#include "MagickImplode.h"
#include "MagickEdge.h"
#include "MagickMotionBlur.h"
#include "MagickTile.h"

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getMagickTextPluginID(ids);
            getMagickSwirlPluginID(ids);
            getMagickModulatePluginID(ids);
            getMagickMirrorPluginID(ids);
            getMagickImplodePluginID(ids);
            getMagickEdgePluginID(ids);
            getMagickMotionBlurPluginID(ids);
            getMagickTilePluginID(ids);
        }
    }
}
