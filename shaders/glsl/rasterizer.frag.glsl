#version 460

layout(location=0) in vec3 in_nrm;
layout(location=1) in vec2 in_uv;

layout(set=2, binding=0) uniform sampler2D diffuse;
layout(set=2, binding=1) uniform sampler2D normal;

layout(location=0) out vec4 out_color;

void main()
{
   out_color = texture(diffuse, in_uv);
}