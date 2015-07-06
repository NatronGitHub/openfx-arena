#include "ofxsImageEffect.h"
#include "Sketch.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getSketchPluginID(ids);
    }
  }
}
