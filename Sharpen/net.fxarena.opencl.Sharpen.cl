/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
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

#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)
#pragma OPENCL EXTENSION cl_amd_fp64 : enable
#endif

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

kernel void filter(read_only image2d_t input, write_only image2d_t output, double factor) {
    const int2 p = {get_global_id(0), get_global_id(1)};
    float m[3][3] = { {-1, -1, -1}, {-1,  8, -1}, {-1, -1, -1} };
    float4 value = 0.f;
    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            value += read_imagef(input, sampler, (int2)(p.x+i, p.y+j)) * m[i+1][j+1] * (float)factor;
        }
    }
  float4 orig = read_imagef(input, sampler, (int2)(p.x, p.y));
  write_imagef(output, (int2)(p.x, p.y), orig+value/m[1][1]);
}

