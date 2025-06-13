#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_nrm;

// scene level
layout(set = 0, binding = 0) uniform VP
{
    mat4 vp;
}
vp;

// prim level
layout(set = 2, binding = 0) uniform M
{
    mat4 m;
}
m;

layout(location = 0) out vec4 out_pos;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec3 out_nrm;

void main()
{
    gl_Position = vp.vp * m.m * vec4(in_position, 1);
    out_pos = m.m * vec4(in_position, 1);
    out_uv = in_uv;
    out_nrm = in_nrm;
}