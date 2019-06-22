const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
void kernel filter(__read_only image2d_t input, __write_only image2d_t output, double _bloom_strength, double _bloom_size_x, double _bloom_size_y)
{ 
  const int2 size = get_image_dim(input);

    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    float4 srcColor = read_imagef(input, sampler, coord);
    
    float avg = (srcColor.x + srcColor.y + srcColor.z) / 3.0f;
    
    float3 sum = (float3)(0,0,0);
    
    float mask_width = (float)_bloom_size_x;
    float mask_height = (float)_bloom_size_y;
    
    float bloom_strength = (float)_bloom_strength;
    
    for (int i = -mask_width;i < mask_width; i++) {
        for (int j = -mask_height; j < mask_height; j++) {
            sum += read_imagef(input, sampler, coord + (int2)(i, j)).xyz * bloom_strength;
        }
    }
    
    float3 dstRGB;
    
    if (avg < 0.025f) {
        dstRGB = srcColor.xyz + sum * 0.335f;
    } else if ( avg < 0.10f) {
        dstRGB = srcColor.xyz + (sum * sum) * 0.5f;
    } else if (avg < 0.88f) {
        dstRGB = srcColor.xyz + (sum * sum) * 0.333f;
    } else if (avg >= 0.88f) {
        dstRGB = srcColor.xyz + sum;
    } else {
        dstRGB = srcColor.xyz;
    }
    
    write_imagef(output, coord, (float4)(dstRGB, 1.0f));
}

