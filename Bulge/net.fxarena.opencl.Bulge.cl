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

const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

kernel void filter(__read_only image2d_t input, __write_only image2d_t output, double centerX, double centerY, double r, double effect) {
   const int2 size = get_image_dim(input);
   int2 coord = (int2)(get_global_id(0),get_global_id(1));
   int2 center = (int2)((int)centerX,(int)centerY);
   float2 coord_center = convert_float2(coord - center);
   float radius = r / 2.0f;
   float strength = (float)effect;
   float dist = length(convert_float2(coord_center));

   if (dist < radius) {
         float percent = dist / radius;
         if (strength > 0.0f) {
              coord_center *= mix(1.0f, smoothstep(0.0f, radius / dist, percent), strength * 0.75f);
         } else {
              coord_center *= mix(1.0f, pow(percent, 1.0f + strength * 0.75f) * radius / dist, 1.0f - percent);
         }
    }
    coord_center += convert_float2(center);
    float4 color = read_imagef(input,sampler,convert_int2(coord_center));
    write_imagef(output,convert_int2(coord),color);
}
