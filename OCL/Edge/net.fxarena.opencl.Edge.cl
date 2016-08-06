/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
 * Copyright (C) 2015, 2016 FxArena DA
 * Copyright (C) 2016 INRIA
 *
 * openfx-arena is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * openfx-arena is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-arena.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
*/

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
kernel void filter(read_only image2d_t input, write_only image2d_t output) {
   const int2 p = {get_global_id(0), get_global_id(1)};
   float m[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };
   float2 t = {0.f, 0.f};
   for (int j = -1; j <= 1; j++) {
      for (int i = -1; i <= 1; i++) {
          float4 pix = read_imagef(input, sampler, (int2)(p.x+i, p.y+j));
          t.x += (pix.x*0.299f + pix.y*0.587f + pix.z*0.114f) * m[i+1][j+1];
          t.y += (pix.x*0.299f + pix.y*0.587f + pix.z*0.114f) * m[j+1][i+1];
      }
   }
   float o = sqrt(t.x*t.x + t.y*t.y);
   write_imagef(output, p, (float4)(o, o, o, 1.0f));
}

