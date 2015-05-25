#include "ofxsImageEffect.h"
#include "Texture.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getTexturePluginID(ids);
    }
  }
}
