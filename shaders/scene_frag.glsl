#version 460

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color_0;

void main()
{
     out_color_0 = vec4(in_uv, 0, 1);
}