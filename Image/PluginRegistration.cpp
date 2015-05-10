#include "ofxsImageEffect.h"
#include "Text.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getTextPluginID(ids);
    }
  }
}
