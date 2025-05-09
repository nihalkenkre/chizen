#version 460

#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec4 in_pos;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_nrm;

// material level
layout(set = 1, binding = 0) uniform sampler2D base_color;
layout(set = 1, binding = 1) uniform sampler2D metal_rough;
layout(set = 1, binding = 2) uniform Factors
{
    float[4] base_color_factor;
    float metal_factor;
    float rough_factor;
}
factors;

layout(location = 0) out vec4 out_color;

void main()
{
    float dotp = dot(in_nrm, normalize((vec3(10, 10, -10) - in_pos.xyz)));
    out_color = vec4((texture(base_color, in_uv).xyz * max(0, dotp)), 1);
}