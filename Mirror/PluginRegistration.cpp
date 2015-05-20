#include "ofxsImageEffect.h"
#include "Mirror.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getMirrorPluginID(ids);
    }
  }
}
