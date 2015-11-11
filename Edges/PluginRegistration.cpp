#include "ofxsImageEffect.h"
#include "Edges.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getEdgesPluginID(ids);
    }
  }
}
