#include "ofxsImageEffect.h"
#include "Tile.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getTilePluginID(ids);
    }
  }
}
