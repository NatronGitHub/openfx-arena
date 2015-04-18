#include "ofxsImageEffect.h"
#include "MagickText.h"
#include "MagickSwirl.h"
#include "MagickModulate.h"
#include "MagickMirror.h"

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
    }
  }
}
