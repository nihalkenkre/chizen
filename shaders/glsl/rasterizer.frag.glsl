#version 460

layout(location=0) in vec3 in_nrm;
layout(location=1) in vec2 in_uv;

layout(set=2, binding=0) uniform sampler2D base_color_factor;
layout(set=2, binding=1) uniform sampler2D base_color_tex;
layout(set=2, binding=2) uniform sampler2D normal_tex;

layout(location=0) out vec4 out_color;

void main()
{
   vec4 base_color = texture(base_color_tex, in_uv);
   if (base_color == vec4(0.f))
   {
      base_color = texture(base_color_factor, in_uv);
   }
   else
   {
      base_color *= texture(base_color_factor, in_uv);
   }

   vec4 normal_color = texture(normal_tex, in_uv);
   if (normal_color == vec4(0.f))
   {
      normal_color = vec4(in_nrm, 1.f);
   }

   out_color = base_color;
}