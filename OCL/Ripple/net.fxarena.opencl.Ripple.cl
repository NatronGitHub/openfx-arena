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

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;
int2 effect(float x, float y, float a, float w) {
        float m = sqrt(x*x + y*y);
        float s = sin(w*m);
        float x1 = a*y*m*s;
        float y1 = -a*x*m*s;
        return (int2)(x1, y1);
}
int2 displacement(int xi, int yi, int width, int height, double waveAmp, double waveLength) {
        float aspect = ((float) height) / ((float) width);
        float x = ((float) xi) / ((float) width), y = ((float) yi) / ((float) height);
        return effect(x - 0.5, aspect*(y - 0.5 + 0.35), waveAmp, waveLength)
             - effect(x - 0.5, aspect*(y - 0.5 - 0.35), waveAmp, waveLength)
             + effect(x - 0.5, aspect*(y), 2, 5000);
}
kernel void filter(read_only image2d_t inputImage, write_only image2d_t outputImage, double waveAmp, double waveLength) {
   int2 dimensions = get_image_dim(inputImage);
   int width = dimensions.x, height = dimensions.y;
   int channelDataType = get_image_channel_data_type(inputImage);
   int channelOrder = get_image_channel_order(inputImage);
   int x = get_global_id(0), y = get_global_id(1);
   int2 coordinates = (int2)(x, y);
   int2 disp = displacement(x, y, width, height, waveAmp, waveLength);
   int2 fromCoordinates = coordinates + disp;
   float4 pixel = read_imagef(inputImage, sampler, fromCoordinates);
   float4 transformedPixel = pixel;
   write_imagef(outputImage, coordinates, transformedPixel);
}

