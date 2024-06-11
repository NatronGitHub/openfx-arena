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

void CairoHelper::applyRotate(cairo_t *cr,
                              double rotate,
                              XY origin)
{
    if (!cr || rotate == 0.) { return; }

    cairo_translate(cr, origin.x, origin.y);
    cairo_rotate(cr, -rotate * (M_PI / 180.0));
    cairo_translate(cr, -origin.x, -origin.y);
}

void CairoHelper::applyFlip(cairo_t *cr,
                            int height)
{
    if (!cr || height < 1) { return; }

    cairo_scale(cr, 1.0f, -1.0f);
    cairo_translate(cr, 0.0f, -height);
}
