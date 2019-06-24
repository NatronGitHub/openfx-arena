const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
void kernel filter(__read_only image2d_t input, __write_only image2d_t output, double _amount)
{ 
   const int2 size = get_image_dim(input);

   int2 coord = (int2)(get_global_id(0),get_global_id(1));
   
   float4 color = read_imagef(input,sampler,coord);
   
   float amount = (float)_amount;
  
   float average = (color.x + color.y + color.z) / 3.0f;
   float mx = max(color.x, max(color.y, color.z));
   float amt = (mx - average) * (-amount * 3.0f);
   color.xyz = mix(color.xyz, (float3)(mx,mx,mx), amt);
   
   write_imagef(output,coord,color);
}

