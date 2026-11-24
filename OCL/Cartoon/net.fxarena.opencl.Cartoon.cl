/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
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

const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

float luma(float4 color) {
  return dot(color.xyz, (float3)(0.2126f, 0.7152f, 0.0722f));
}

kernel void filter(__read_only image2d_t input, __write_only image2d_t output){

  const int2 coord = (int2)(get_global_id(0),get_global_id(1));
  const float4 color = read_imagef(input,sampler,coord);

  // centred 3x3 Sobel on the luma, with integer pixel offsets (unnormalized coordinates)
  const float sobel[3][3] = { {-1.0f, 0.0f, 1.0f}, {-2.0f, 0.0f, 2.0f}, {-1.0f, 0.0f, 1.0f} }; // [j][i], d/dx
  float gx = 0.0f;
  float gy = 0.0f;
  for (int j = -1; j <= 1; j++) {
    for (int i = -1; i <= 1; i++) {
      const float l = luma(read_imagef(input,sampler,coord + (int2)(i,j)));
      gx += l * sobel[j+1][i+1];
      gy += l * sobel[i+1][j+1];
    }
  }
  float edge = min(fabs(gx) + fabs(gy), 1.0f);

  write_imagef(output,coord,(float4)(color.xyz - (float3)(edge), color.w));
}
