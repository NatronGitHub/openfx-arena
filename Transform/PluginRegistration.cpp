#include "ofxsImageEffect.h"
#include "Mirror.h"
#include "Reflection.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getMirrorPluginID(ids);
        getReflectionPluginID(ids);
    }
  }
}
