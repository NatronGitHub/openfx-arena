#include "ofxsImageEffect.h"
#include "TextPango.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getTextPangoPluginID(ids);
    }
  }
}
