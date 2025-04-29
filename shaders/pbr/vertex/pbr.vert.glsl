#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

// scene level
layout(set = 0, binding = 0) uniform VP {
    mat4 vp;
} vp;

// prim level
layout(set = 1, binding = 0) uniform M {
    mat4 m;
} m;

layout(location = 0) out vec2 out_uv;

void main() {
    gl_Position = vp.vp * m.m * vec4(in_position, 1);
    out_uv = in_uv;
}