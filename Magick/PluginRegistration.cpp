#include "ofxsImageEffect.h"
#include "MagickText.h"
#include "MagickSwirl.h"
#include "MagickModulate.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getMagickTextPluginID(ids);
        getMagickSwirlPluginID(ids);
	getMagickModulatePlugin(ids);
    }
  }
}
