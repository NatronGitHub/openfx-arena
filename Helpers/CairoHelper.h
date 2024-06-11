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

#ifndef CAIROHELPER_H
#define CAIROHELPER_H

#include <cairo.h>

class CairoHelper
{
public:
    struct _XY
    {
        double x;
        double y;
    };
    struct _Transform
    {
        _XY origin;
        _XY scale;
        _XY skew;
        int width;
        int height;
        double rotate;
        bool position;
        bool flip;
    };
    /** @brief apply flip */
    static void applyFlip(cairo_t *cr,
                          const int &height);
    /** @brief apply position */
    static void applyPosition(cairo_t *cr,
                              const _XY &position);
    /** @brief apply scale */
    static void applyScale(cairo_t *cr,
                           const _XY &scale);
    /** @brief apply skew */
    static void applySkew(cairo_t *cr,
                          const _XY &skew,
                          const _XY &origin);
    /** @brief apply rotate */
    static void applyRotate(cairo_t *cr,
                            const double &rotate,
                            const _XY &origin);
    /** @brief apply transform */
    static void applyTransform(cairo_t *cr,
                               const _Transform &transform);
};

#endif // CAIROHELPER_H
