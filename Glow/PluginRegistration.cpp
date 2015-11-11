#include "ofxsImageEffect.h"
#include "Glow.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getGlowPluginID(ids);
    }
  }
}
