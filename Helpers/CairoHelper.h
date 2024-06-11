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
    struct XY
    {
        double x;
        double y;
    };
    /** @brief apply rotate */
    static void applyRotate(cairo_t *cr,
                            double rotate,
                            XY origin);
    /** @brief apply flip */
    static void applyFlip(cairo_t *cr,
                          int height);
};

#endif // CAIROHELPER_H
