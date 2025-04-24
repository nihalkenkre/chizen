#version 460

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform VP {
    mat4 vp;
} vp;

layout(set = 0, binding = 1) uniform M {
    mat4 m;
} m;

void main() {
    gl_Position = vp.vp * m.m * vec4(in_position, 1);
}