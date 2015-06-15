#include "ofxsImageEffect.h"
#include "Oilpaint.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getOilpaintPluginID(ids);
    }
  }
}
