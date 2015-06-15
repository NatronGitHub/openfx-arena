#include "ofxsImageEffect.h"
#include "Swirl.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getSwirlPluginID(ids);
    }
  }
}
