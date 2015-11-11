#include "ofxsImageEffect.h"
#include "PovRay.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getPovRayPluginID(ids);
    }
  }
}
