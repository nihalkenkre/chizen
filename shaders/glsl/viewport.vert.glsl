#version 460

layout(location=0) in vec3 in_pos;
layout(location=1) in vec3 in_nrm;
layout(location=2) in vec2 in_uv;

layout(set=0, binding=0) uniform ViewProj
{
   mat4 view_proj;
} view_proj;

layout(set=1, binding=0) uniform Model
{
   mat4 model;
} model;

layout(location=0) out vec3 out_nrm;
layout(location=1) out vec2 out_uv;

void main()
{
   gl_Position = view_proj.view_proj * model.model * vec4(in_pos, 1);
   out_uv = in_uv;
   out_nrm = normalize((transpose(inverse(model.model)) * vec4(in_nrm, 1)).xyz);
}