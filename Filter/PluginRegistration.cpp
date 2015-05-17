#include "ofxsImageEffect.h"
#include "Swirl.h"
#include "Implode.h"
#include "UnsharpMask.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getSwirlPluginID(ids);
        getImplodePluginID(ids);
        getUnsharpMaskPluginID(ids);
    }
  }
}