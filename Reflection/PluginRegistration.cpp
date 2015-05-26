#include "ofxsImageEffect.h"
#include "Reflection.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getReflectionPluginID(ids);
    }
  }
}
