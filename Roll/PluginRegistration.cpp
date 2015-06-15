#include "ofxsImageEffect.h"
#include "Roll.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getRollPluginID(ids);
    }
  }
}
