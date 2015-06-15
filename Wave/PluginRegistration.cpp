#include "ofxsImageEffect.h"
#include "Wave.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getWavePluginID(ids);
    }
  }
}
