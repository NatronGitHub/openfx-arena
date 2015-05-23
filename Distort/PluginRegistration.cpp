#include "ofxsImageEffect.h"
#include "Distort.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getDistortPluginID(ids);
    }
  }
}
