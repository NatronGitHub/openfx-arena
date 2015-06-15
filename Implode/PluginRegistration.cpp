#include "ofxsImageEffect.h"
#include "Implode.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getImplodePluginID(ids);
    }
  }
}
