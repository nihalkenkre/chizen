#version 460

layout(location=0) in vec2 in_uv;
layout(location=1) in vec3 in_nrm;

layout(location=0) out vec4 out_color;

void main()
{
   out_color = vec4(in_nrm, 1);
}