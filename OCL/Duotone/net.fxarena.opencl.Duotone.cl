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

const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
kernel void filter(__read_only image2d_t input, __write_only image2d_t output, double dr, double dg, double db, double lr, double lg, double lb) {
   const int2 dim = get_image_dim(input);
   int2 coord = (int2)(get_global_id(0),get_global_id(1));
   float4 color = read_imagef(input,sampler,coord);

   float3 dark_color = (float3)(dr,dg,db);
   float3 light_color = (float3)(lr,lg,lb);
   float gray = dot(color.xyz, (float3)(0.2126f, 0.7152f, 0.0722f));
   float luminance = dot(color.xyz,gray);

   color.xyz = clamp(mix(dark_color,light_color,luminance),0.0f,1.0f);
   write_imagef(output,coord,color);
}

