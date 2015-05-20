#include "ofxsImageEffect.h"
#include "Swirl.h"
#include "Implode.h"
#include "UnsharpMask.h"
#include "Polar.h"
#include "Arc.h"

namespace OFX 
{
  namespace Plugin 
  {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        getSwirlPluginID(ids);
        getImplodePluginID(ids);
        getUnsharpMaskPluginID(ids);
        getPolarPluginID(ids);
        getArcPluginID(ids);
    }
  }
}
