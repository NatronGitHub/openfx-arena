/*
####################################################################
#
# Copyright (C) 2024 Ole-André Rodlie <https://github.com/rodlie>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
####################################################################
*/

#include "CairoHelper.h"

#include <cmath>

void
CairoHelper::applyPosition(cairo_t *cr,
                           const _XY &position,
                           const bool translate)
{
    if (!cr) { return; }
    if (translate) {
        cairo_translate(cr, position.x, position.y);
    } else {
        cairo_move_to(cr, position.x, position.y);
    }
}

void
CairoHelper::applyScale(cairo_t *cr,
                        const _XY &scale)
{
    if (!cr || (scale.x == 1. && scale.y == 1.)) { return; }
    cairo_scale(cr, scale.x, scale.y);
}

void
CairoHelper::applyScale(cairo_t *cr,
                        const _XY &scale,
                        const _XY &origin)
{
    if (!cr || (scale.x == 1. && scale.y == 1.)) { return; }
    cairo_translate(cr, origin.x, origin.y);
    cairo_scale(cr, scale.x, scale.y);
    cairo_translate(cr, -origin.x, -origin.y);
}

void
CairoHelper::applySkew(cairo_t *cr,
                       const _XY &skew,
                       const _XY &origin)
{
    if (!cr || (skew.x == 0. && skew.y == 0.)) { return; }
    cairo_translate(cr, origin.x, origin.y);
    if (skew.x != 0.) {
        cairo_matrix_t matrix = {
            1.0, 0.0,
            skew.x, 1.0,
            0.0, 0.0
        };
        cairo_transform(cr, &matrix);
    }
    if (skew.y != 0.) {
        cairo_matrix_t matrix = {
            1.0, skew.y,
            0.0, 1.0,
            0.0, 0.0
        };
        cairo_transform(cr, &matrix);
    }
    cairo_translate(cr, -origin.x, -origin.y);
}

void
CairoHelper::applyRotate(cairo_t *cr,
                         const double &rotate,
                         const _XY &origin)
{
    if (!cr || rotate == 0.) { return; }
    cairo_translate(cr, origin.x, origin.y);
    cairo_rotate(cr, rotate * (M_PI / 180.0));
    cairo_translate(cr, -origin.x, -origin.y);
}

void
CairoHelper::applyTransform(cairo_t *cr,
                            const _Transform &transform)
{
    if (!cr) { return; }
    if (transform.position) { applyPosition(cr,
                                            transform.translate,
                                            true); }
    applyScale(cr,
               transform.scale,
               transform.origin);
    applySkew(cr,
              transform.skew,
              transform.origin);
    applyRotate(cr,
                transform.rotate,
                transform.origin);
}