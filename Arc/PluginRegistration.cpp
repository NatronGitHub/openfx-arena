#include "ofxsImageEffect.h"
#include "Arc.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getArcPluginID(ids);
    }
  }
}
