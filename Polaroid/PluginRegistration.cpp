#include "ofxsImageEffect.h"
#include "Polaroid.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getPolaroidPluginID(ids);
    }
  }
}
