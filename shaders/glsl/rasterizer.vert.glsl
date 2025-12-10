#version 460

layout(location=0) in vec3 in_pos;
layout(location=1) in vec2 in_uv;
layout(location=2) in vec3 in_nrm;

layout(set=0, binding=0) uniform ViewProj
{
   mat4 view;
   mat4 proj;
} view_proj;

layout(set=1, binding=0) uniform Model
{
   mat4 model;
} model;

layout(location=0) out vec2 out_uv;
layout(location=1) out vec3 out_nrm;

void main()
{
   gl_Position = view_proj.proj * view_proj.view * model.model * vec4(in_pos, 1);
   out_uv = in_uv;
   out_nrm = in_nrm;
}