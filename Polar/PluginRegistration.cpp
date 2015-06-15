#include "ofxsImageEffect.h"
#include "Polar.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getPolarPluginID(ids);
    }
  }
}
