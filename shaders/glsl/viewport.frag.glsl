#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec3 in_nrm;
layout(location=1) in vec2 in_uv;

layout(set=2, binding=0) uniform sampler2D base_color_factor;
layout(set=2, binding=1) uniform sampler2D base_color_tex;
layout(set=2, binding=2) uniform sampler2D normal_tex;

struct Material {
   vec4 base_color_factor;
   ivec4 base_color_tex;
};

layout(set=2, binding=3) readonly buffer mat_buff
{
   Material materials[];
} materials;

layout(set=2, binding=4) uniform sampler2D textures[];

layout(push_constant) uniform PushConstants {
   int material_index;
} pc;

layout(location=0) out vec4 out_color;

void main()
{
   // vec4 base_color = texture(base_color_tex, in_uv);
   // if (base_color == vec4(0.f))
   // {
   //    base_color = texture(base_color_factor, in_uv);
   // }
   // else
   // {
   //    base_color *= texture(base_color_factor, in_uv);
   // }

   // vec4 normal_color = texture(normal_tex, in_uv);
   // if (normal_color == vec4(0.f))
   // {
   //    normal_color = vec4(in_nrm, 1.f);
   // }

   vec4 base_color = vec4(1);

   if (pc.material_index >= 0)
   {
      if (materials.materials[pc.material_index].base_color_tex.x >= 0)
      {
         base_color *= texture(textures[nonuniformEXT(materials.materials[pc.material_index].base_color_tex.x)], in_uv) * materials.materials[pc.material_index].base_color_factor;
      }
      else
      {
         base_color *= materials.materials[pc.material_index].base_color_factor;
      }
   }

   out_color = base_color;
}