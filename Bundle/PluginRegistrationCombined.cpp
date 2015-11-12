#include "ofxsImageEffect.h"
#include "Texture.h"
#include "Tile.h"
#include "Reflection.h"
#include "Text.h"
#include "Arc.h"
#include "Polar.h"
#include "Roll.h"
#include "Wave.h"
#include "Swirl.h"
#include "Implode.h"
#include "Charcoal.h"
#include "Oilpaint.h"
#include "TextPango.h"
#include "ReadPSD.h"
#include "ReadSVG.h"
#include "Sketch.h"
#include "Polaroid.h"
#include "Edges.h"
#include "Modulate.h"

#if !defined(_WIN32) && !defined(__MINGW__)
#define USE_POV_RAY
#include "PovRay.h"
#endif

namespace OFX
{
    namespace Plugin
    {
        void getPluginIDs(OFX::PluginFactoryArray &ids)
        {
            getTexturePluginID(ids);
            getTilePluginID(ids);
            getReflectionPluginID(ids);
            getTextPluginID(ids);
            getArcPluginID(ids);
            getPolarPluginID(ids);
            getRollPluginID(ids);
            getWavePluginID(ids);
            getSwirlPluginID(ids);
            getImplodePluginID(ids);
            getCharcoalPluginID(ids);
            getOilpaintPluginID(ids);
            getTextPangoPluginID(ids);
            getReadPSDPluginID(ids);
            getReadSVGPluginID(ids);
            getSketchPluginID(ids);
            getPolaroidPluginID(ids);
            getEdgesPluginID(ids);
            getModulatePluginID(ids);
            #ifdef USE_POV_RAY
            getPovRayPluginID(ids);
            #endif
        }
    }
}
