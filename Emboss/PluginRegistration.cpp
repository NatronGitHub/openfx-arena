#include "ofxsImageEffect.h"
#include "Emboss.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getEmbossPluginID(ids);
    }
  }
}
