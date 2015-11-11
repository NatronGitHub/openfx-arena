#include "ofxsImageEffect.h"
#include "Modulate.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getModulatePluginID(ids);
    }
  }
}
