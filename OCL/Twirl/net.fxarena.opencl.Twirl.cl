/*
 * This file is part of openfx-arena <https://github.com/olear/openfx-arena>,
 * Copyright (C) 2015, 2016 FxArena DA
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

// CLAMP_TO_EDGE: the bicubic stencil reads up to two pixels outside the image
const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

float clampRGB( float x ) {
    return x > 1.f ? 1.f
         : x < 0.f   ? 0.f
         : x;
}

kernel void filter (read_only image2d_t in, write_only image2d_t out, float centerW, float centerH, float amount, float radius) {

    int2 d = get_image_dim(in);
    int2 pos = (int2)(get_global_id(0),get_global_id(1));
    float x = pos.x - centerW;
    float y = pos.y - centerH;

    // a radius of 0 twirls nothing: -(x*x+y*y) / 0 gave NaN at the center (0 / 0) and
    // exp(-inf) = 0 elsewhere
    float r2 = radius*radius;
    float a = r2 > 0.f ? amount*exp(-(x*x+y*y)/r2) : 0.f;
    float u = (cos(a)*x + sin(a)*y);
    float v = (-sin(a)*x + cos(a)*y);

    u += centerW;
    v += centerH;

    // Interpolation
    int2 p11 = (int2)(floor(u),floor(v));
    float dx = u-(float)p11.x;
    float dy = v-(float)p11.y;

    float4 C[4];
    float4 d0,d2,d3,a0,a1,a2,a3;

    // rows and columns p11-1..p11+2 around p11 = floor(u, v)
    for (int i = 0; i < 4; i++) {
        int row = p11.y-1+i;
        d0 = read_imagef(in,sampler,(int2)(p11.x-1,row)) - read_imagef(in,sampler,(int2)(p11.x,row));
        d2 = read_imagef(in,sampler,(int2)(p11.x+1,row)) - read_imagef(in,sampler,(int2)(p11.x,row));
        d3 = read_imagef(in,sampler,(int2)(p11.x+2,row)) - read_imagef(in,sampler,(int2)(p11.x,row));
        a0 = read_imagef(in,sampler,(int2)(p11.x,  row));
        a1 =  -1.0f/3.f*d0 + d2 - 1.0f/6.f*d3;
        a2 = 1.0f/2.f*d0 + 1.0f/2.f*d2;
        a3 = -1.0f/6.f*d0 - 1.0f/2.f*d2 + 1.0f/6.f*d3;

        C[i] = a0 + a1*dx + a2*dx*dx + a3*dx*dx*dx;
    }

    d0 = C[0]-C[1];
    d2 = C[2]-C[1];
    d3 = C[3]-C[1];
    a0 = C[1];
    a1 = -1.0f/3.f*d0 + d2 -1.0f/6.f*d3;
    a2 = 1.0f/2.f*d0 + 1.0f/2.f*d2;
    a3 = -1.0f/6.f*d0 - 1.0f/2.f*d2 + 1.0f/6.f*d3;
    float4 fp = (float4)(a0 + a1*dy + a2*dy*dy + a3*dy*dy*dy);
    fp.x = clampRGB(fp.x);
    fp.y = clampRGB(fp.y);
    fp.z = clampRGB(fp.z);
    fp.w = clampRGB(fp.w);

    write_imagef(out,(int2)(pos.x,pos.y),fp);
}

