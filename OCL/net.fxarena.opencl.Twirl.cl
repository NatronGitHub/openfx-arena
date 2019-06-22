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

#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)
#pragma OPENCL EXTENSION cl_amd_fp64 : enable
#endif

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;

float clampRGB( float x ) {
    return x > 1.f ? 1.f
         : x < 0.f   ? 0.f
         : x;
}

kernel void filter (read_only image2d_t in, write_only image2d_t out, double centerW, double centerH, double amount, double radius) {

    int2 d = get_image_dim(in);
    int2 pos = (int2)(get_global_id(0),get_global_id(1));
    int x = pos.x - centerW;
    int y = pos.y - centerH;

    float a = amount*exp(-(x*x+y*y)/(radius*radius));
    float u = (cos(a)*x + sin(a)*y);
    float v = (-sin(a)*x + cos(a)*y);

    u += (float)centerW;
    v += (float)centerH;

    float4 fp = read_imagef(in,sampler,(int2)((int)u,(int)v));

    // Interpolation
    int2 p11 = (int2)(floor(u),floor(v));
    float dx = u-(float)p11.x;
    float dy = v-(float)p11.y;

    float4 C[5];
    float4 d0,d2,d3,a0,a1,a2,a3;

    for (int i = 0; i < 4; i++) {
        d0 = read_imagef(in,sampler,(int2)((int)u-1,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));
        d2 = read_imagef(in,sampler,(int2)((int)u+1,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));
        d3 = read_imagef(in,sampler,(int2)((int)u+2,(int)v+i)) - read_imagef(in,sampler,(int2)((int)u,(int)v+i));
        a0 = read_imagef(in,sampler,(int2)((int)u,  (int)v+i));
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
    fp = (float4)(a0 + a1*dy + a2*dy*dy + a3*dy*dy*dy);
    fp.x = clampRGB(fp.x);
    fp.y = clampRGB(fp.y);
    fp.z = clampRGB(fp.z);
    fp.w = clampRGB(fp.w);

    write_imagef(out,(int2)(pos.x,pos.y),fp);
}

