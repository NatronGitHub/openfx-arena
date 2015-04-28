#include "ofxsImageEffect.h"
#include "GmicGeneric.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getGmicGenericPluginID(ids);
    }
  }
}
