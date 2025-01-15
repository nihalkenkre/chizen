#version 460

layout(set = 0, binding = 0) uniform mat_block_1
{
    mat4 proj;
    mat4 view;
}
cam_mats;

layout(set = 1, binding = 0) uniform mat_block_2
{
    mat4 model;
}
obj_mats;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_nrm;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

void main()
{
    gl_Position = cam_mats.proj * cam_mats.view * obj_mats.model * vec4(in_pos, 1);

    out_uv = in_uv;
}