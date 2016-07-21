/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-supportext <https://github.com/devernay/openfx-supportext>,
 * Copyright (C) 2013-2016 INRIA
 *
 * openfx-supportext is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-supportext is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-supportext.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

/*
 * OFX TransformInteractCustom.
 */

#ifndef openfx_supportext_ofxsTransformInteractCustom_h
#define openfx_supportext_ofxsTransformInteractCustom_h

#include <cmath>

#include "ofxsImageEffect.h"
#include "ofxsMacros.h"

#define kParamTransformTranslate "transformTranslate"
#define kParamTransformTranslateLabel "Translate"
#define kParamTransformRotate "transformRotate"
#define kParamTransformRotateLabel "Rotate"
#define kParamTransformScale "transformScale"
#define kParamTransformScaleLabel "Scale"
#define kParamTransformScaleUniform "transformScaleUniform"
#define kParamTransformScaleUniformLabel "Uniform"
#define kParamTransformScaleUniformHint "Use the X scale for both directions"
/*#define kParamTransformSkewX "transformSkewX"
#define kParamTransformSkewXLabel "Skew X"
#define kParamTransformSkewY "transformSkewY"
#define kParamTransformSkewYLabel "Skew Y"
#define kParamTransformSkewOrder "transformSkewOrder"
#define kParamTransformSkewOrderLabel "Skew Order"*/
#define kParamTransformCenter "transformCenter"
#define kParamTransformCenterLabel "Center"
#define kParamTransformResetCenter "transformResetCenter"
#define kParamTransformResetCenterLabel "Reset Center"
#define kParamTransformResetCenterHint "Reset the position of the center to the center of the input region of definition"
#define kParamTransformInteractCustomOpen "TransformInteractCustomOpen"
#define kParamTransformInteractCustomOpenLabel "Show Interact"
#define kParamTransformInteractCustomOpenHint "If checked, the transform interact is displayed over the image."
#define kParamTransformInteractCustomive "TransformInteractCustomive"
#define kParamTransformInteractCustomiveLabel "Interactive Update"
#define kParamTransformInteractCustomiveHint "If checked, update the parameter values during interaction with the image viewer, else update the values when pen is released."

// old parameter names (Transform DirBlur and GodRays only)
#define kParamTransformTranslateOld "translate"
#define kParamTransformRotateOld "rotate"
#define kParamTransformScaleOld "scale"
#define kParamTransformScaleUniformOld "uniform"
/*#define kParamTransformSkewXOld "skewX"
#define kParamTransformSkewYOld "skewY"
#define kParamTransformSkewOrderOld "skewOrder"*/
#define kParamTransformCenterOld "center"
#define kParamTransformResetCenterOld "resetCenter"
#define kParamTransformInteractCustomiveOld "interactive"

namespace OFX {
inline void
ofxsTransformGetScale(const OfxPointD &scaleParam,
                      bool scaleUniform,
                      OfxPointD* scale)
{
    const double SCALE_MIN = 0.0001;

    scale->x = scaleParam.x;
    if (std::fabs(scale->x) < SCALE_MIN) {
        scale->x = (scale->x >= 0) ? SCALE_MIN : -SCALE_MIN;
    }
    if (scaleUniform) {
        scale->y = scaleParam.x;
    } else {
        scale->y = scaleParam.y;
    }
    if (std::fabs(scale->y) < SCALE_MIN) {
        scale->y = (scale->y >= 0) ? SCALE_MIN : -SCALE_MIN;
    }
}

/// add Transform params. page and group are optional
void ofxsTransformDescribeParams(OFX::ImageEffectDescriptor &desc, OFX::PageParamDescriptor *page, OFX::GroupParamDescriptor *group, bool isOpen, bool oldParams, bool noTranslate = false);

class TransformInteractCustomHelper
    : private OFX::InteractAbstract
{
protected:
    enum DrawStateEnum
    {
        eInActive = 0, //< nothing happening
        eCircleHovered, //< the scale circle is hovered
        eLeftPointHovered, //< the left point of the circle is hovered
        eRightPointHovered, //< the right point of the circle is hovered
        eBottomPointHovered, //< the bottom point of the circle is hovered
        eTopPointHovered, //< the top point of the circle is hovered
        eCenterPointHovered, //< the center point of the circle is hovered
        eRotationBarHovered, //< the rotation bar is hovered
        //eSkewXBarHoverered, //< the skew bar is hovered
        //eSkewYBarHoverered //< the skew bar is hovered
    };

    enum MouseStateEnum
    {
        eReleased = 0,
        eDraggingCircle,
        eDraggingLeftPoint,
        eDraggingRightPoint,
        eDraggingTopPoint,
        eDraggingBottomPoint,
        eDraggingTranslation,
        eDraggingCenter,
        eDraggingRotationBar,
        //eDraggingSkewXBar,
        //eDraggingSkewYBar
    };

    enum OrientationEnum
    {
        eOrientationAllDirections = 0,
        eOrientationNotSet,
        eOrientationHorizontal,
        eOrientationVertical
    };

    DrawStateEnum _drawState;
    MouseStateEnum _mouseState;
    int _modifierStateCtrl;
    int _modifierStateShift;
    OrientationEnum _orientation;
    ImageEffect* _effect;
    Interact* _interact;
    OfxPointD _lastMousePos;
    OfxPointD _centerDrag;
    OfxPointD _translateDrag;
    OfxPointD _scaleParamDrag;
    bool _scaleUniformDrag;
    double _rotateDrag;
    /*double _skewXDrag;
    double _skewYDrag;
    int _skewOrderDrag;*/
    bool _invertedDrag;
    bool _interactiveDrag;

public:
    TransformInteractCustomHelper(OFX::ImageEffect* effect, OFX::Interact* interact, bool oldParams = false);

    /** @brief virtual destructor */
    virtual ~TransformInteractCustomHelper()
    {
        // fetched clips and params are owned and deleted by the ImageEffect and its ParamSet
    }

    // overridden functions from OFX::Interact to do things
    virtual bool draw(const OFX::DrawArgs &args) OVERRIDE;
    virtual bool penMotion(const OFX::PenArgs &args) OVERRIDE;
    virtual bool penDown(const OFX::PenArgs &args) OVERRIDE;
    virtual bool penUp(const OFX::PenArgs &args) OVERRIDE;
    virtual bool keyDown(const OFX::KeyArgs &args) OVERRIDE;
    virtual bool keyUp(const OFX::KeyArgs &args) OVERRIDE;
    virtual bool keyRepeat(const KeyArgs & /*args*/) OVERRIDE { return false; }

    virtual void gainFocus(const FocusArgs & /*args*/) OVERRIDE {}

    virtual void loseFocus(const FocusArgs &args) OVERRIDE;

private:
    // NON-GENERIC
    OFX::Double2DParam* _translate;
    OFX::DoubleParam* _rotate;
    OFX::Double2DParam* _scale;
    OFX::BooleanParam* _scaleUniform;
    /*OFX::DoubleParam* _skewX;
    OFX::DoubleParam* _skewY;
    OFX::ChoiceParam* _skewOrder;*/
    OFX::Double2DParam* _center;
    OFX::BooleanParam* _invert;
    OFX::BooleanParam* _interactOpen;
    OFX::BooleanParam* _interactive;
};

typedef OverlayInteractFromHelper<TransformInteractCustomHelper> TransformInteractCustom;

class TransformOverlayDescriptor
    : public DefaultEffectOverlayDescriptor<TransformOverlayDescriptor, TransformInteractCustom>
{
};

class TransformInteractCustomHelperOldParams
    : public TransformInteractCustomHelper
{
public:
    TransformInteractCustomHelperOldParams(OFX::ImageEffect* effect,
                                     OFX::Interact* interact)
        : TransformInteractCustomHelper(effect, interact, true) {}
};

typedef OverlayInteractFromHelper<TransformInteractCustomHelperOldParams> TransformInteractCustomOldParams;

class TransformOverlayDescriptorOldParams
    : public DefaultEffectOverlayDescriptor<TransformOverlayDescriptorOldParams, TransformInteractCustomOldParams>
{
};
} // namespace OFX
#endif /* defined(openfx_supportext_ofxsTransformInteractCustom_h) */
