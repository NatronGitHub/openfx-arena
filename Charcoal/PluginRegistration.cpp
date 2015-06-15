#include "ofxsImageEffect.h"
#include "Charcoal.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getCharcoalPluginID(ids);
    }
  }
}
