/*
 * openfx-arena <https://github.com/rodlie/openfx-arena>,
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-arena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
*/

#include <librsvg/rsvg.h>

#ifdef LEGACY
#include "librsvg/rsvg-cairo.h"
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <cmath>
#include <iostream>
#include <sys/stat.h>

#include "ofxNatron.h"
#include "GenericReader.h"
#include "GenericOCIO.h"
#include "PackedImage.h"
#include "ofxsMacros.h"
#include "ofxsMultiPlane.h"
#include "ofxsImageEffect.h"
#include "ofxsMatrix2D.h"
#include "ofxsTransformInteract.h"
#include "ofxsMultiThread.h"
#ifndef OFX_USE_MULTITHREAD_MUTEX
#include "fast_mutex.h"
#endif

#define kPluginName "ReadSVG"
#define kPluginGrouping "Image/Readers"
#define kPluginIdentifier "net.fxarena.openfx.ReadSVG"
// history:
// 3.3 the cairo buffer given by rsvg is actually premultiplied (Natron 2.5.0)
// 3.4 transform (translate, rotate, scale, skew), with an overlay (unreleased, now 4.0 only)
// 3.5 obsolete: 3.3 (no transform, no overlay, the DPI applied twice to physical units) with the
//     bug fixes made since: bounds and row bytes, no assert at render scales below 1, the NaN
//     guards and the empty canvas error. 3.5 rather than 3.4, which had the transform.
// 4.0 the DPI is applied once to documents in physical units (mm, in, pt...), see getCanvasSize();
//     the transform and its overlay of 3.4
#define kPluginVersionMajor 4
#define kPluginVersionMinor 0
#define kPluginVersionMajorV3 3
#define kPluginVersionMinorV3 5
#define kPluginEvaluation 50

#define kParamDpi "dpi"
#define kParamDpiLabel "DPI"
#define kParamDpiHint "Dots-per-inch (90 is default)"
#define kParamDpiDefault 90

#define kGroupTransform "transformGroup"
#define kGroupTransformLabel "Transform"
#define kGroupTransformHint "Transform the SVG before it is rasterized, so that it stays sharp. " \
    "The result is clipped to the SVG canvas."
#define kParamTransformCenterHintSVG "Center of rotation and scale. While it is at the center of the SVG canvas (as in a new node), " \
    "it follows the center of the SVG canvas, without moving the image, when the file or the DPI change. " \
    "Setting it elsewhere, by editing it or by moving it in the viewer, stops that; Reset Center makes it follow the canvas again."

// Hidden: whether the transform center follows the SVG canvas (see autoCenter()). A flag rather
// than a sentinel center value, so that the user can put the center anywhere, (0,0) included.
#define kParamAutoCenter "autoCenter"
#define kParamAutoCenterLabel "Auto Center"
#define kParamAutoCenterHint "The transform center follows the center of the SVG canvas. Cleared when the center is set elsewhere, set again when it is back at the canvas center or by Reset Center."

#define kSupportsRGBA true
#define kSupportsRGB false
#define kSupportsXY false
#define kSupportsAlpha false
#define kSupportsTiles false
#define kIsMultiPlanar true

using namespace OFX;
using namespace OFX::IO;

#ifdef OFX_IO_USING_OCIO
namespace OCIO = OCIO_NAMESPACE;
#endif

OFXS_NAMESPACE_ANONYMOUS_ENTER

static bool gHostIsNatron = false;

// the nanoseconds of the modification and status change times, 0 where struct stat doesn't have them
static long
statMtimeNsec(const struct stat& st)
{
#if defined(__APPLE__)
    return st.st_mtimespec.tv_nsec;
#elif defined(_WIN32)
    (void)st;
    return 0;
#else
    return st.st_mtim.tv_nsec;
#endif
}

static long
statCtimeNsec(const struct stat& st)
{
#if defined(__APPLE__)
    return st.st_ctimespec.tv_nsec;
#elif defined(_WIN32)
    (void)st;
    return 0;
#else
    return st.st_ctim.tv_nsec;
#endif
}

// as in GenericOCIO.h
#ifdef OFX_USE_MULTITHREAD_MUTEX
typedef OFX::MultiThread::Mutex Mutex;
typedef OFX::MultiThread::AutoMutex AutoMutex;
#else
typedef tthread::fast_mutex Mutex;
typedef OFX::MultiThread::AutoMutexT<tthread::fast_mutex> AutoMutex;
#endif

// The size of the SVG canvas, and how to render it into an output image of width x height pixels
struct SVGCanvas {
    double imageWidth, imageHeight; // the document size, in pixels at the DPI of the handle
    int width, height; // the output size, in pixels
    double scaleX, scaleY; // from the pixels of the handle to the output pixels
    bool exactViewport; // render into an imageWidth x imageHeight viewport (rsvg_handle_render_document)
    SVGCanvas() : imageWidth(0.), imageHeight(0.), width(0), height(0), scaleX(1.), scaleY(1.), exactViewport(false) {}
};

class ReadSVGPlugin : public GenericReaderPlugin
{
public:
    ReadSVGPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions, unsigned int majorVersion);
    virtual ~ReadSVGPlugin();
    virtual void restoreStateFromParams() OVERRIDE FINAL;
    virtual void changedParam(const OFX::InstanceChangedArgs &args, const std::string &paramName) OVERRIDE FINAL;
    virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip* &identityClip, double &identityTime, int& view, std::string& plane) OVERRIDE FINAL;
private:
    virtual bool isOutputFrameVarying() const OVERRIDE FINAL { return isTransformAnimated(); }
    virtual bool isVideoStream(const std::string& /*filename*/) OVERRIDE FINAL { return false; }
    virtual void decode(const std::string& filename, OfxTime time, int view, const OfxRectI& renderWindow, const OfxPointD& renderScale, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, int pixelComponentCount, int rowBytes) OVERRIDE FINAL
    {
        std::string rawComps;
        switch (pixelComponents) {
            case OFX::ePixelComponentAlpha:
                rawComps = kOfxImageComponentAlpha;
                break;
            case OFX::ePixelComponentRGB:
                rawComps = kOfxImageComponentRGB;
                break;
            case OFX::ePixelComponentRGBA:
                rawComps = kOfxImageComponentRGBA;
                break;
            default:
                OFX::throwSuiteStatusException(kOfxStatFailed);
                return;
        }
        decodePlane(filename, time, view, renderWindow, renderScale, pixelData, bounds, pixelComponents, pixelComponents, pixelComponentCount, rawComps, rowBytes);
    }
    virtual void decodePlane(const std::string& filename, OfxTime time, int view, const OfxRectI& renderWindow, const OfxPointD& renderScale, float *pixelData, const OfxRectI& bounds, OFX::PixelComponentEnum pixelComponents, OFX::PixelComponentEnum remappedComponents, int pixelComponentCount, const std::string& rawComponents, int rowBytes) OVERRIDE FINAL;
    virtual OfxStatus getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents) OVERRIDE FINAL;
    virtual bool getFrameBounds(const std::string& filename, OfxTime time, int view, OfxRectI *bounds, OfxRectI* format, double *par, std::string *error, int *tile_width, int *tile_height) OVERRIDE FINAL;
    virtual bool guessParamsFromFilename(const std::string& filename, std::string *colorspace, OFX::PreMultiplicationEnum *filePremult, OFX::PixelComponentEnum *components, int *componentCount) OVERRIDE FINAL;
    virtual void changedFilename(const OFX::InstanceChangedArgs &args) OVERRIDE FINAL;
    virtual void clearAnyCache() OVERRIDE FINAL;
    void getLayers(xmlNode *node, std::vector<std::string> *layers);
    bool isTransformAnimated() const;
    // the transform from the untransformed canvas to the output, in canonical coordinates
    bool getTransformCanonical(double time, OFX::Matrix3x3* transform) const;
    bool getCanvasCenter(double time, OfxPointD* center);
    void resetCenter(double time, bool keepImageFixed);
    void autoCenter(double time);
    void getCanvasSize(RsvgHandle* handle, int dpi, SVGCanvas* canvas) const;
    const bool _v4; // version 4 or later: the DPI fix and the transform (see the history)
    OFX::IntParam *_dpi;
    // the transform params: NULL in version 3, which doesn't have them
    OFX::Double2DParam *_translate;
    OFX::DoubleParam *_rotate;
    OFX::Double2DParam *_scale;
    OFX::BooleanParam *_scaleUniform;
    OFX::DoubleParam *_skewX;
    OFX::DoubleParam *_skewY;
    OFX::ChoiceParam *_skewOrder;
    OFX::Double2DParam *_center;
    OFX::BooleanParam *_autoCenter;
    std::vector<std::string> imageLayers;
    // The canvas size from the last getFrameBounds(), so that it doesn't parse the whole SVG each
    // time: GenericReader calls it from getClipPreferences(), which the host calls again at each
    // change of a transform param (they are clip preferences slaves), e.g. while dragging the overlay.
    struct CanvasSize {
        std::string filename;
        int dpi;
        // the file: atomic saves replace the inode; mtime has a 1 s resolution, so also the
        // nanoseconds where available, and ctime
        dev_t dev;
        ino_t ino;
        time_t mtime, ctime;
        long mtimeNsec, ctimeNsec;
        off_t size;
        int width, height;
        CanvasSize() : dpi(0), dev(0), ino(0), mtime(0), ctime(0), mtimeNsec(0), ctimeNsec(0), size(0), width(0), height(0) {}
        void setFile(const struct stat& st)
        {
            dev = st.st_dev;
            ino = st.st_ino;
            mtime = st.st_mtime;
            ctime = st.st_ctime;
            mtimeNsec = statMtimeNsec(st);
            ctimeNsec = statCtimeNsec(st);
            size = st.st_size;
        }
        bool sameFile(const struct stat& st) const
        {
            return dev == st.st_dev && ino == st.st_ino && mtime == st.st_mtime && ctime == st.st_ctime &&
                   mtimeNsec == statMtimeNsec(st) && ctimeNsec == statCtimeNsec(st) && size == st.st_size;
        }
    };
    Mutex _canvasSizeMutex; // getFrameBounds() is also called from render(), possibly concurrently
    CanvasSize _canvasSize;
};

ReadSVGPlugin::ReadSVGPlugin(OfxImageEffectHandle handle, const std::vector<std::string>& extensions, unsigned int majorVersion)
: GenericReaderPlugin(handle, extensions, kSupportsRGBA, kSupportsRGB, kSupportsXY, kSupportsAlpha, kSupportsTiles,
#ifdef OFX_EXTENSIONS_NUKE
(OFX::getImageEffectHostDescription() && OFX::getImageEffectHostDescription()->isMultiPlanar) ? kIsMultiPlanar : false
#else
false
#endif
)
,_v4(majorVersion >= 4)
,_dpi(NULL)
,_translate(NULL)
,_rotate(NULL)
,_scale(NULL)
,_scaleUniform(NULL)
,_skewX(NULL)
,_skewY(NULL)
,_skewOrder(NULL)
,_center(NULL)
,_autoCenter(NULL)
{
    _dpi = fetchIntParam(kParamDpi);
    assert(_dpi);
    if (!_v4) {
        return;
    }
    _translate = fetchDouble2DParam(kParamTransformTranslate);
    _rotate = fetchDoubleParam(kParamTransformRotate);
    _scale = fetchDouble2DParam(kParamTransformScale);
    _scaleUniform = fetchBooleanParam(kParamTransformScaleUniform);
    _skewX = fetchDoubleParam(kParamTransformSkewX);
    _skewY = fetchDoubleParam(kParamTransformSkewY);
    _skewOrder = fetchChoiceParam(kParamTransformSkewOrder);
    _center = fetchDouble2DParam(kParamTransformCenter);
    _autoCenter = fetchBooleanParam(kParamAutoCenter);
    assert(_translate && _rotate && _scale && _scaleUniform && _skewX && _skewY && _skewOrder && _center && _autoCenter);
}

ReadSVGPlugin::~ReadSVGPlugin()
{
}

void ReadSVGPlugin::restoreStateFromParams()
{
    GenericReaderPlugin::restoreStateFromParams();

    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st == kOfxStatOK || !filename.empty() ) {
        imageLayers.clear();
        xmlDocPtr doc;
        doc = xmlParseFile(filename.c_str());
        xmlNode *root_element = NULL;
        root_element = xmlDocGetRootElement(doc);
        getLayers(root_element,&imageLayers);
        xmlFreeDoc(doc);
    }

    // The file may already be set at instance creation, without a later instanceChanged: on project
    // load, and when Natron's Read node switches to this decoder with its filename already set
    // (Natron sends no instanceChanged for the filename then, before Natron#1284). Center the
    // transform on the canvas then too.
    autoCenter(startingTime);
}

bool
ReadSVGPlugin::isTransformAnimated() const
{
    if (!_v4) {
        return false;
    }
    return (_translate->getNumKeys() > 0 || _rotate->getNumKeys() > 0 || _scale->getNumKeys() > 0 ||
            _scaleUniform->getNumKeys() > 0 || _skewX->getNumKeys() > 0 || _skewY->getNumKeys() > 0 ||
            _skewOrder->getNumKeys() > 0 || _center->getNumKeys() > 0);
}

// Returns false if the transform is the identity, or in version 3, which has no transform.
// Same conventions and matrix as the Transform node (openfx-misc) and its overlay (ofxsTransformInteract).
bool
ReadSVGPlugin::getTransformCanonical(double time, Matrix3x3* transform) const
{
    if (!_v4) {
        return false;
    }
    OfxPointD translate, scaleParam, center;
    _translate->getValueAtTime(time, translate.x, translate.y);
    _scale->getValueAtTime(time, scaleParam.x, scaleParam.y);
    const bool scaleUniform = _scaleUniform->getValueAtTime(time);
    const double rotate = _rotate->getValueAtTime(time);
    const double skewX = _skewX->getValueAtTime(time);
    const double skewY = _skewY->getValueAtTime(time);
    const int skewOrder = _skewOrder->getValueAtTime(time);
    _center->getValueAtTime(time, center.x, center.y);
    OfxPointD scale;
    ofxsTransformGetScale(scaleParam, scaleUniform, &scale);
    if (translate.x == 0. && translate.y == 0. && rotate == 0. && scale.x == 1. && scale.y == 1. && skewX == 0. && skewY == 0.) {
        return false;
    }
    *transform = ofxsMatTransformCanonical(translate.x, translate.y, scale.x, scale.y, skewX, skewY, (bool)skewOrder,
                                           ofxsToRadians(rotate), center.x, center.y);
    return true;
}

// The center of the SVG canvas, i.e. of the RoD, in canonical coordinates. False if there is no
// readable file.
bool
ReadSVGPlugin::getCanvasCenter(double time, OfxPointD* center)
{
    std::string filename;
    OfxStatus st = getFilenameAtTime(getStartingTime(), &filename);
    if (st != kOfxStatOK || filename.empty()) {
        return false;
    }
    OfxRectI bounds, format;
    double par = 1.;
    std::string error;
    int tile_width, tile_height;
    try {
        if (!getFrameBounds(filename, time, 0, &bounds, &format, &par, &error, &tile_width, &tile_height) ||
            format.x2 <= format.x1 || format.y2 <= format.y1) {
            return false;
        }
    } catch (...) {
        // getFrameBounds() throws if the file can't be read (and sets a persistent message)
        return false;
    }
    // canonical coordinates: x is scaled by the PAR, see the OpenFX spec on coordinate systems. Use
    // the output clip's PAR, as GenericReader's RoD does (the host may not use the one from getFrameBounds)
    par = _outputClip->getPixelAspectRatio();
    center->x = (format.x1 + format.x2) * par / 2.;
    center->y = (format.y1 + format.y2) / 2.;
    return true;
}

// Unless the user set the center, move it to the center of the canvas, without moving the image:
// called whenever the canvas is known or changes (instance creation, file and DPI changes).
void
ReadSVGPlugin::autoCenter(double time)
{
    if (!_v4 || !_autoCenter->getValue()) {
        return;
    }
    resetCenter(time, true);
}

// Set the center to the center of the SVG canvas. If keepImageFixed, also change the translation
// so that the image doesn't move (as the Transform node's Reset Center).
void
ReadSVGPlugin::resetCenter(double time, bool keepImageFixed)
{
    OfxPointD newCenter;
    if (!getCanvasCenter(time, &newCenter)) {
        return;
    }
    OfxPointD center;
    _center->getValueAtTime(time, center.x, center.y);
    if (newCenter.x == center.x && newCenter.y == center.y) {
        return;
    }
    OfxPointD translate;
    _translate->getValueAtTime(time, translate.x, translate.y);
    OfxPointD newTranslate = translate;
    if (keepImageFixed) {
        OfxPointD scaleParam;
        _scale->getValueAtTime(time, scaleParam.x, scaleParam.y);
        OfxPointD scale;
        ofxsTransformGetScale(scaleParam, _scaleUniform->getValueAtTime(time), &scale);
        const double rot = ofxsToRadians(_rotate->getValueAtTime(time));
        const Matrix3x3 Rinv = (ofxsMatRotation(-rot) *
                                ofxsMatSkewXY(_skewX->getValueAtTime(time), _skewY->getValueAtTime(time), _skewOrder->getValueAtTime(time) != 0) *
                                ofxsMatScale(scale.x, scale.y));
        const double dxrot = newCenter.x - center.x;
        const double dyrot = newCenter.y - center.y;
        Point3D dRot(dxrot, dyrot, 1.);
        dRot = Rinv * dRot;
        if (dRot.z != 0) {
            dRot.x /= dRot.z;
            dRot.y /= dRot.z;
        }
        newTranslate.x = translate.x + dRot.x - dxrot;
        newTranslate.y = translate.y + dRot.y - dyrot;
    }
    EditBlock eb(*this, "resetCenter", newTranslate.x != translate.x || newTranslate.y != translate.y);
    _center->setValue(newCenter.x, newCenter.y);
    if (newTranslate.x != translate.x || newTranslate.y != translate.y) {
        _translate->setValue(newTranslate.x, newTranslate.y);
    }
}

void
ReadSVGPlugin::changedParam(const InstanceChangedArgs &args, const std::string &paramName)
{
    if (!_v4) {
        // no transform params
    } else if (paramName == kParamTransformResetCenter) {
        resetCenter(args.time, true);
        // follow the canvas again (after resetCenter, whose setValue comes back as a plugin edit)
        if (!_autoCenter->getValue()) {
            _autoCenter->setValue(true);
        }
    } else if (paramName == kParamTransformCenter && args.reason != eChangeTime) {
        // Follow the canvas exactly when the center is at the canvas center, whatever the reason:
        // the reason can't tell the user from the plug-in. Natron reports undo/redo and Restore
        // Defaults as user edits (they would otherwise leave a reset node not following the
        // canvas), the overlay (TransformInteract) moves the center as a plugin edit, and our own
        // setValue in resetCenter() may come back later than it is made (Natron defers
        // instanceChanged inside edit blocks and during createInstance). Without a canvas (no file
        // yet), only a user edit counts.
        OfxPointD center, canvasCenter;
        _center->getValueAtTime(args.time, center.x, center.y);
        bool follow;
        if (getCanvasCenter(args.time, &canvasCenter)) {
            follow = (center.x == canvasCenter.x && center.y == canvasCenter.y);
        } else {
            follow = _autoCenter->getValue() && args.reason != eChangeUserEdit;
        }
        if (follow != _autoCenter->getValue()) {
            _autoCenter->setValue(follow);
        }
    } else if (paramName == kParamDpi && args.reason != eChangeTime) {
        // the canvas size changed
        autoCenter(args.time);
    }
    GenericReaderPlugin::changedParam(args, paramName);
}

bool
ReadSVGPlugin::isIdentity(const IsIdentityArguments &args, Clip* &identityClip, double &identityTime, int& view, std::string& plane)
{
    // For a held still image, GenericReader's isIdentity() says that the output is the one at
    // another time (only on Natron; on other hosts it returns false). That is wrong when the
    // transform is animated. Natron's Read node doesn't forward isIdentity to the reader at the
    // time of writing, so this is for hosts that use the reader directly and for future versions.
    if (isTransformAnimated()) {
        return false;
    }
    return GenericReaderPlugin::isIdentity(args, identityClip, identityTime, view, plane);
}

void
ReadSVGPlugin::getLayers(xmlNode *node, std::vector<std::string> *layers)
{
    xmlNode *cur_node = NULL;
    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"g"))) {
                xmlChar *xmlID;
                xmlID = xmlGetProp(cur_node, (const xmlChar *)"id");
                if (xmlID!=NULL) {
                    std::string layerName;
                    layerName = (reinterpret_cast<char*>(xmlID));
                    layers->push_back(layerName);
                }
                xmlFree(xmlID);
            }
            else if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"path"))) {
                xmlChar *xmlID;
                xmlID = xmlGetProp(cur_node, (const xmlChar *)"id");
                if (xmlID!=NULL) {
                    std::string layerName;
                    layerName = (reinterpret_cast<char*>(xmlID));
                    layers->push_back(layerName);
                }
                xmlFree(xmlID);
            }
        }
        getLayers(cur_node->children,layers);
    }
}

OfxStatus
ReadSVGPlugin::getClipComponents(const OFX::ClipComponentsArguments& args, OFX::ClipComponentsSetter& clipComponents)
{
    assert(isMultiPlanar());
    clipComponents.setPassThroughClip(NULL, args.time, args.view);
    if (imageLayers.size()>0 && gHostIsNatron) {
        for (int i = 0; i < (int)imageLayers.size(); i++) {
            if (!imageLayers[i].empty()) {

                std::string layerName;
                {
                    std::ostringstream ss;
                    ss << imageLayers[i];
                    layerName = ss.str();
                }
                const char* components[4] = {"R","G","B", "A"};
                OFX::MultiPlane::ImagePlaneDesc plane(layerName, layerName, "", components, 4);
                clipComponents.addClipPlane(*_outputClip, OFX::MultiPlane::ImagePlaneDesc::mapPlaneToOFXPlaneString(plane));
            }
        }

        // Also add the color plane
        clipComponents.addClipPlane(*_outputClip, OFX::MultiPlane::ImagePlaneDesc::mapPlaneToOFXPlaneString(OFX::MultiPlane::ImagePlaneDesc::getRGBAComponents()));
    }
    return kOfxStatOK;
}

// The size of the document with the handle at the given DPI, rounded by librsvg to whole pixels.
// The handle stays at that DPI.
static void
getDimensionsAtDpi(RsvgHandle* handle, int dpi, RsvgDimensionData* dimension)
{
    rsvg_handle_set_dpi_x_y(handle, dpi, dpi);
    rsvg_handle_get_dimensions(handle, dimension);
}

// Get the size of the canvas at the given DPI, and leave the handle at the DPI to render with. The
// DPI scales the whole document, whatever its units: documents in user units (px) were already
// scaled by dpi / 90 in version 3, and they keep that.
void
ReadSVGPlugin::getCanvasSize(RsvgHandle* handle, int dpi, SVGCanvas* canvas) const
{
    if (!_v4) {
        // Version 3 (3.3, Natron 2.5.0): librsvg converts the physical units (mm, in, pt...) to pixels
        // at the DPI of the handle, and the result is scaled by dpi / 90 once more, so the DPI applies
        // twice to them (an image in mm is 4 times as large at 180 DPI as at 90). Kept for old projects.
        RsvgDimensionData dimension;
        getDimensionsAtDpi(handle, dpi, &dimension);
        canvas->imageWidth = dimension.width;
        canvas->imageHeight = dimension.height;
        if (dpi != kParamDpiDefault) {
            canvas->width = canvas->imageWidth * dpi / kParamDpiDefault;
            canvas->height = canvas->imageHeight * dpi / kParamDpiDefault;
        } else {
            canvas->width = canvas->imageWidth;
            canvas->height = canvas->imageHeight;
        }
        // no scale for an empty canvas (0 / 0): decodePlane() fails on it
        canvas->scaleX = (canvas->imageWidth > 0.) ? canvas->width / canvas->imageWidth : 1.;
        canvas->scaleY = (canvas->imageHeight > 0.) ? canvas->height / canvas->imageHeight : 1.;
        canvas->exactViewport = false;
        return;
    }
    // Version 4: render with the handle at the default DPI, at which librsvg converts the physical
    // units, so that the elements in physical units in the document stay consistent with the canvas,
    // and scale everything to the size at the given DPI, once. Unlike version 3, which truncated the
    // size and scaled by the truncated size over the size at 90 DPI, the size is rounded to the
    // nearest pixel and the scale is dpi / 90: documents in user units render as in version 3 only
    // when dpi / 90 is an integer (400 x 200 px at 300 DPI: 1333 x 666 in version 3, 1333 x 667 here).
    const double scale = (double)dpi / kParamDpiDefault;
#if LIBRSVG_CHECK_VERSION(2, 52, 0)
    // The exact size, at 90 DPI, scaled by dpi / 90 (100mm is 354.33 pixels at 90 DPI, 708.66 at
    // 180 DPI: 709 pixels, not 2 * 354), and rendered into a viewport of that exact size.
    rsvg_handle_set_dpi_x_y(handle, kParamDpiDefault, kParamDpiDefault);
    gdouble w, h;
    if (rsvg_handle_get_intrinsic_size_in_pixels(handle, &w, &h)) {
        canvas->imageWidth = w;
        canvas->imageHeight = h;
        canvas->width = (int)std::floor(w * scale + 0.5);
        canvas->height = (int)std::floor(h * scale + 0.5);
        canvas->scaleX = canvas->scaleY = scale;
        canvas->exactViewport = true;
        return;
    }
#endif
    // An older librsvg (Natron's Linux SDK has 2.40), or no width and height in the document (only
    // a viewBox): rsvg_handle_get_dimensions() only gives the size rounded to whole pixels, and
    // rsvg_handle_render_cairo() renders into a viewport of that size. Scaling the size at 90 DPI by
    // dpi / 90 would multiply its rounding error (up to dpi / 180 pixels: 4720 instead of 4724 for
    // 100mm at 1200 DPI). So also measure at the given DPI: where the size changes with the DPI,
    // that axis is in physical units, and librsvg's size at that DPI is the right one, rounded once;
    // the render scale maps the 90 DPI viewport onto it. Elsewhere (user units), scale by dpi / 90.
    RsvgDimensionData atDpi, at90;
    getDimensionsAtDpi(handle, dpi, &atDpi);
    getDimensionsAtDpi(handle, kParamDpiDefault, &at90); // last: the handle renders at 90 DPI
    canvas->imageWidth = at90.width;
    canvas->imageHeight = at90.height;
    if (atDpi.width != at90.width && at90.width > 0) {
        canvas->width = atDpi.width;
        canvas->scaleX = (double)atDpi.width / at90.width;
    } else {
        canvas->width = (int)std::floor(at90.width * scale + 0.5);
        canvas->scaleX = scale;
    }
    if (atDpi.height != at90.height && at90.height > 0) {
        canvas->height = atDpi.height;
        canvas->scaleY = (double)atDpi.height / at90.height;
    } else {
        canvas->height = (int)std::floor(at90.height * scale + 0.5);
        canvas->scaleY = scale;
    }
    canvas->exactViewport = false;
}

void
ReadSVGPlugin::decodePlane(const std::string& filename, OfxTime time, int /*view*/, const OfxRectI& renderWindow, const OfxPointD& renderScale, float *pixelData, const OfxRectI& bounds,
                                 OFX::PixelComponentEnum /*pixelComponents*/, OFX::PixelComponentEnum remappedComponents, int pixelComponentCount, const std::string& rawComponents, int rowBytes)
{
    // renderWindow is at full resolution, whatever renderScale: when renderScale is not 1, GenericReader
    // passes it unchanged and downscales the decoded image (OpenFX-IO GenericReader::render)
    unused(renderScale);
    if (pixelComponentCount != 4) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Wrong pixel components");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    if (remappedComponents != OFX::ePixelComponentRGBA) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Wrong pixel format");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    std::string layerID;
    if (gHostIsNatron) {
        OFX::MultiPlane::ImagePlaneDesc plane, pairedPlane;
        OFX::MultiPlane::ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(rawComponents, &plane, &pairedPlane);
        layerID = plane.getPlaneLabel();
        if (plane.isColorPlane()) {
            layerID.clear();
        }
    }

    GError *error = NULL;
    RsvgHandle *handle;
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_status_t status;
    int dpi, width, height;
    _dpi->getValueAtTime(time, dpi);

    handle = rsvg_handle_new_from_file(filename.c_str(), &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    SVGCanvas canvas;
    getCanvasSize(handle, dpi, &canvas);
    width = canvas.width;
    height = canvas.height;
    // The whole canvas is rendered, at (0,0,width,height) as in getFrameBounds(), and only the part
    // of renderWindow that it covers is copied: a host may ask for a render window smaller than
    // the image, even without tiles (#105). This used to fail unless renderWindow was the canvas.
    OfxRectI image;
    image.x1 = 0;
    image.y1 = 0;
    image.x2 = width;
    image.y2 = height;

    // An empty canvas (a zero width or height) gave 0 / 0 = NaN scales below, which only
    // cairo's check of the matrix turned into the "Cairo Render failed" error: fail here instead.
    // (width or height 0 used to fail as "Image don't match RenderWindow")
    if (canvas.imageWidth <= 0. || canvas.imageHeight <= 0. || width <= 0 || height <= 0) {
        g_object_unref(handle);
        setPersistentMessage(OFX::Message::eMessageError, "", "Cairo Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr=cairo_create(surface);

    // the transform params animate: evaluate them at the render time, not at the time in the file
    // sequence (`time`), which is constant for a still image
    Matrix3x3 transform;
    if (getTransformCanonical(getRenderTime(), &transform)) {
        // The surface covers the whole image, at full resolution (see above), top-down: from
        // canonical coordinates, x is divided by the PAR, and y goes down from image.y2.
        // (not from renderWindow, which may be only a part of the image: the transformed canvas
        // must not depend on it)
        // a PAR <= 0 (not a valid host value) would make 1 / par and the matrices below inf/NaN
        const double clipPar = _outputClip->getPixelAspectRatio();
        const double par = (clipPar > 0.) ? clipPar : 1.;
        const Matrix3x3 canonicalToSurface(1. / par, 0., -image.x1,
                                           0., -1., image.y2,
                                           0., 0., 1.);
        const Matrix3x3 surfaceToCanonical(par, 0., image.x1 * par,
                                           0., -1., image.y2,
                                           0., 0., 1.);
        const Matrix3x3 m = canonicalToSurface * transform * surfaceToCanonical;
        cairo_matrix_t cm;
        cairo_matrix_init(&cm, m(0, 0), m(1, 0), m(0, 1), m(1, 1), m(0, 2), m(1, 2));
        cairo_transform(cr, &cm);
    }
    cairo_scale(cr, canvas.scaleX, canvas.scaleY);

    std::string layerSub;
    if (!layerID.empty()) {
        layerSub = "#" + layerID;
    }
#if LIBRSVG_CHECK_VERSION(2, 52, 0)
    if (canvas.exactViewport) {
        // version 4: rsvg_handle_render_cairo() renders into a viewport of the size rounded to whole
        // pixels, which shrinks the document a little (100mm: 354 pixels instead of 354.33 at 90
        // DPI); the scale would multiply that error. Render into the exact size instead.
        RsvgRectangle viewport;
        viewport.x = 0.;
        viewport.y = 0.;
        viewport.width = canvas.imageWidth;
        viewport.height = canvas.imageHeight;
        GError *renderError = NULL;
        // as rsvg_handle_render_cairo(), the result is ignored: an error leaves the image empty
        if (layerSub.empty()) {
            rsvg_handle_render_document(handle, cr, &viewport, &renderError);
        } else {
            rsvg_handle_render_layer(handle, cr, layerSub.c_str(), &viewport, &renderError);
        }
        if (renderError) {
            g_error_free(renderError);
        }
    } else
#endif
    if (layerSub.empty()) {
        rsvg_handle_render_cairo(handle, cr);
    }
    else {
        rsvg_handle_render_cairo_sub(handle, cr, layerSub.c_str());
    }

    status = cairo_status(cr);

    if (status) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Cairo Render failed");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    cairo_surface_flush(surface);

    unsigned char* cdata = cairo_image_surface_get_data(surface);
    const int cstride = cairo_image_surface_get_stride(surface);
    // The surface covers image, top-down: the OFX row y is the surface row height - 1 - y.
    // Copy the part of renderWindow inside the image (clamped, so that a window larger than the
    // image or outside of it doesn't read out of the surface), and clear the rest of the window.
    // pixelData has the given bounds and rowBytes, which may differ from renderWindow, be padded,
    // or be negative.
    // GenericReader's clipToFile() does both (copied is empty if the window is not in the
    // image), and tells render() that only copied comes from the file: call it once per decode(),
    // with decode()'s renderWindow (see GenericReader.h)
    OfxRectI copied;
    clipToFile(renderWindow, image, pixelData, bounds, 4, OFX::eBitDepthFloat, rowBytes, &copied);
    for (int y = copied.y1; y < copied.y2; ++y) {
        size_t cdataOffset = (size_t)(height - 1 - y) * cstride + (size_t)copied.x1 * 4;
        float* dst = PackedImage::pixelAddress(pixelData, bounds, rowBytes, 4, copied.x1, y);
        for (int x = copied.x1; x < copied.x2; ++x) {
#if 0
            // check on premulttest.svg that the raw data is premultiplied
            // See also TestImageSVGPremult in Natron-Tests github repository.
            if (x == 200 && ((y % 50) == 25)) {
                printf("x=%d y=%d r=%d g=%d b=%d a=%d\n", x, y, cdata[cdataOffset + 2], cdata[cdataOffset + 1], cdata[cdataOffset + 0], cdata[cdataOffset + 3]);
            }
#endif
            dst[0] = intToFloat<256>(cdata[cdataOffset + 2]);
            dst[1] = intToFloat<256>(cdata[cdataOffset + 1]);
            dst[2] = intToFloat<256>(cdata[cdataOffset + 0]);
            dst[3] = intToFloat<256>(cdata[cdataOffset + 3]);
            dst += 4;
            cdataOffset += 4;
        }
    }

    g_object_unref(handle);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cdata = NULL;
    error = NULL;
}

bool ReadSVGPlugin::getFrameBounds(const std::string& filename,
                              OfxTime time,
                                   int /*view*/,
                              OfxRectI *bounds,
                                   OfxRectI* format,
                              double *par,
                              std::string* errorMsg,int *tile_width, int *tile_height)
{
    if (filename.empty()) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    int dpi;
    _dpi->getValueAtTime(time, dpi);

    int width, height;
    // the cache is valid for the same file, unchanged on disk (see CanvasSize), and DPI
    struct stat st;
    const bool statOK = (stat(filename.c_str(), &st) == 0);
    bool cached = false;
    if (statOK) {
        AutoMutex lock(_canvasSizeMutex);
        if (_canvasSize.filename == filename && _canvasSize.dpi == dpi && _canvasSize.sameFile(st)) {
            width = _canvasSize.width;
            height = _canvasSize.height;
            cached = true;
        }
    }
    if (!cached) {
        GError *error = NULL;
        RsvgHandle *handle;

        handle = rsvg_handle_new_from_file(filename.c_str(), &error);

        if (error != NULL) {
            setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
            OFX::throwSuiteStatusException(kOfxStatErrFormat);
        }

        SVGCanvas canvas;
        getCanvasSize(handle, dpi, &canvas);
        width = canvas.width;
        height = canvas.height;

        g_object_unref(handle);
        error = NULL;

        if (statOK) {
            AutoMutex lock(_canvasSizeMutex);
            _canvasSize.filename = filename;
            _canvasSize.dpi = dpi;
            _canvasSize.setFile(st);
            _canvasSize.width = width;
            _canvasSize.height = height;
        }
    }

    if (width <= 0 || height <= 0) {
        // an empty canvas: report it, instead of returning true with the bounds unset, from which
        // GenericReader made a garbage RoD (#129)
        if (errorMsg) {
            *errorMsg = "Empty SVG canvas";
        }
        return false;
    }
    bounds->x1 = 0;
    bounds->x2 = width;
    bounds->y1 = 0;
    bounds->y2 = height;
    *format = *bounds;
    *par = 1.0;

    *tile_width = *tile_height = 0;
    return true;
}

void
ReadSVGPlugin::clearAnyCache()
{
    AutoMutex lock(_canvasSizeMutex);
    _canvasSize = CanvasSize();
}

bool ReadSVGPlugin::guessParamsFromFilename(const std::string& /*newFile*/,
                                       std::string *colorspace,
                                       OFX::PreMultiplicationEnum *filePremult,
                                       OFX::PixelComponentEnum *components,
                                       int *componentCount)
{
    assert(colorspace && filePremult && components && componentCount);
# ifdef OFX_IO_USING_OCIO
    *colorspace = "sRGB";
# endif
    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st != kOfxStatOK || filename.empty() ) {
        return false;
    }

    imageLayers.clear();
    xmlDocPtr doc;
    doc = xmlParseFile(filename.c_str());
    xmlNode *root_element = NULL;
    root_element = xmlDocGetRootElement(doc);
    getLayers(root_element,&imageLayers);
    xmlFreeDoc(doc);

    GError *error = NULL;
    RsvgHandle *handle;

    handle=rsvg_handle_new_from_file(filename.c_str(), &error);

    if (error != NULL) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to read SVG");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    g_object_unref(handle);
    error = NULL;

    *components = OFX::ePixelComponentRGBA;
    // the output of rsvg is premultiplied, see premulttest.svg check above
    // it was premultiplied until 12 Jun 2016 commit https://github.com/NatronGitHub/openfx-arena/commit/7ab22e08ee93cd55b7cdfd6e8dfd831636973477
    // and unpremultiplied until 29 May 2018 commit https://github.com/NatronGitHub/openfx-arena/commit/a645fa74533bbeba83ea6134d0c8c6f649cec49d
    *filePremult = OFX::eImagePreMultiplied;

    return true;
}

void ReadSVGPlugin::changedFilename(const OFX::InstanceChangedArgs &args)
{
    GenericReaderPlugin::changedFilename(args);

    int startingTime = getStartingTime();
    std::string filename;
    OfxStatus st = getFilenameAtTime(startingTime, &filename);
    if ( st != kOfxStatOK || filename.empty() ) {
        setPersistentMessage(OFX::Message::eMessageError, "", "No filename");
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
    imageLayers.clear();
    xmlDocPtr doc;
    doc = xmlParseFile(filename.c_str());
    xmlNode *root_element = NULL;
    root_element = xmlDocGetRootElement(doc);
    getLayers(root_element,&imageLayers);
    xmlFreeDoc(doc);

    // move the center to the center of the new canvas, unless the user moved it, and change the
    // translation so that the transformed image doesn't move (as the Transform node does when its
    // input changes). GenericReader calls changedFilename() for any reason but eChangeTime.
    autoCenter(args.time);
}

using namespace OFX;

// mDeclareReaderPluginFactory, with the major version as a template parameter: each major version
// must be a distinct factory class (see mDeclarePluginFactoryVersioned in ofxsImageEffect.h)
template<unsigned int majorVersion>
class ReadSVGPluginFactory
    : public OFX::PluginFactoryHelper<ReadSVGPluginFactory<majorVersion> >
{
public:
    ReadSVGPluginFactory(const std::string& id, unsigned int verMin)
        : OFX::PluginFactoryHelper<ReadSVGPluginFactory<majorVersion> >(id, majorVersion, verMin)
    {
    }
    virtual void load() OVERRIDE FINAL;
    virtual void unload() OVERRIDE FINAL {}
    virtual OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context) OVERRIDE FINAL;
    bool isVideoStreamPlugin() const { return false; }
    virtual void describe(OFX::ImageEffectDescriptor& desc) OVERRIDE FINAL;
    virtual void describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context) OVERRIDE FINAL;
    std::vector<std::string> _extensions;
};

template<unsigned int majorVersion>
void
ReadSVGPluginFactory<majorVersion>::load()
{
    _extensions.clear();
    _extensions.push_back("svg");
    _extensions.push_back("svgz");
}


/** @brief The basic describe function, passed a plugin descriptor */
template<unsigned int majorVersion>
void
ReadSVGPluginFactory<majorVersion>::describe(OFX::ImageEffectDescriptor &desc)
{
    GenericReaderDescribe(desc, _extensions, kPluginEvaluation, kSupportsTiles, kIsMultiPlanar);
    desc.setLabel(kPluginName);
    if (majorVersion < kPluginVersionMajor) {
        // hidden from the hosts' menus (e.g. Natron), still loaded by old projects
        desc.setIsObsolete(true);
        desc.setPluginDescription("Fast SVG (Scalable Vector Graphics) reader using librsvg and Cairo.");
        return;
    }
    desc.setPluginDescription("Fast SVG (Scalable Vector Graphics) reader using librsvg and Cairo.\n"
                              "The Transform parameters are applied before rasterization, so the SVG stays sharp when it is scaled up. "
                              "The result is clipped to the SVG canvas.");
    // Natron runs readers inside its Read node, which forwards the overlay actions (hasOverlay, draw,
    // pen and key) to the reader from Natron#1284 (RB-2.6); released Natron 2.5 doesn't, so it
    // doesn't show this overlay. The params work everywhere.
    // V2 (OFX 1.5 DrawSuite) on hosts with the DrawSuite, else V1 (OpenGL), as before.
    // ReadSVG 3 returns above, without an overlay.
    ofxsSetOverlayInteractDescriptorV2(desc, new TransformOverlayDescriptorV2);
}

/** @brief The describe in context function, passed a plugin descriptor and a context */
template<unsigned int majorVersion>
void
ReadSVGPluginFactory<majorVersion>::describeInContext(OFX::ImageEffectDescriptor &desc, ContextEnum context)
{
    gHostIsNatron = (OFX::getImageEffectHostDescription()->isNatron);

    PageParamDescriptor *page = GenericReaderDescribeInContextBegin(desc, context, isVideoStreamPlugin(), kSupportsRGBA, kSupportsRGB, kSupportsXY,kSupportsAlpha, kSupportsTiles, false);
    {
        IntParamDescriptor* param = desc.defineIntParam(kParamDpi);
        param->setLabel(kParamDpiLabel);
        param->setHint(kParamDpiHint);
        param->setRange(1, 10000);
        param->setDisplayRange(1, 500);
        param->setDefault(kParamDpiDefault);
        param->setAnimates(false);
        param->setLayoutHint(OFX::eLayoutHintDivider);
        page->addChild(*param);
    }
    if (majorVersion < 4) {
        // version 3 has no transform (it was added in 3.4, which was never released, and is now 4.0)
        GenericReaderDescribeInContextEnd(desc, context, page, "sRGB", "scene_linear");
        return;
    }
    {
        GroupParamDescriptor* group = desc.defineGroupParam(kGroupTransform);
        group->setLabel(kGroupTransformLabel);
        group->setHint(kGroupTransformHint);
        if (page) {
            page->addChild(*group);
        }
        ofxsTransformDescribeParams(desc, page, group, /*isOpen=*/true, /*oldParams=*/false, /*hasAmount=*/false, /*noTranslate=*/false);
        // ofxsTransformDescribeParams gives the center a normalized default, the center of the
        // project, which is not the center of the SVG. Default to (0,0) in canonical coordinates
        // instead: while autoCenter is set, the plug-in sets it to the center of the canvas as soon
        // as the file is known (see autoCenter()).
        Double2DParamDescriptor* center = dynamic_cast<Double2DParamDescriptor*>(desc.getParamDescriptor(kParamTransformCenter));
        assert(center);
        if (center) {
            center->setHint(kParamTransformCenterHintSVG);
            center->setDefaultCoordinateSystem(eCoordinatesCanonical);
            center->setDefault(0., 0.);
        }
        {
            BooleanParamDescriptor* param = desc.defineBooleanParam(kParamAutoCenter);
            param->setLabel(kParamAutoCenterLabel);
            param->setHint(kParamAutoCenterHint);
            param->setDefault(true);
            param->setIsSecretAndDisabled(true);
            param->setAnimates(false);
            param->setEvaluateOnChange(false);
            param->setParent(*group);
            if (page) {
                page->addChild(*param);
            }
        }
    }
    // the output is frame varying when the transform is animated, see isOutputFrameVarying()
    {
        const char* params[] = { kParamTransformTranslate, kParamTransformRotate, kParamTransformScale, kParamTransformScaleUniform,
                                 kParamTransformSkewX, kParamTransformSkewY, kParamTransformSkewOrder, kParamTransformCenter };
        for (std::size_t i = 0; i < sizeof(params) / sizeof(params[0]); ++i) {
            ParamDescriptor* param = desc.getParamDescriptor(params[i]);
            assert(param);
            if (param) {
                desc.addClipPreferencesSlaveParam(*param);
            }
        }
    }

    GenericReaderDescribeInContextEnd(desc, context, page, "sRGB", "scene_linear");
}

/** @brief The create instance function, the plugin must return an object derived from the \ref OFX::ImageEffect class */
template<unsigned int majorVersion>
ImageEffect*
ReadSVGPluginFactory<majorVersion>::createInstance(OfxImageEffectHandle handle,
                                                   ContextEnum /*context*/)
{
    ReadSVGPlugin* ret =  new ReadSVGPlugin(handle, _extensions, majorVersion);
    ret->restoreStateFromParams();
    return ret;
}

static ReadSVGPluginFactory<kPluginVersionMajor> p(kPluginIdentifier, kPluginVersionMinor);
static ReadSVGPluginFactory<kPluginVersionMajorV3> p3(kPluginIdentifier, kPluginVersionMinorV3);
mRegisterPluginFactoryInstance(p)
mRegisterPluginFactoryInstance(p3)

OFXS_NAMESPACE_ANONYMOUS_EXIT
