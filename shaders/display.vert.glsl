#version 460

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;

layout(push_constant) uniform PushConstants {
   vec2 pos_offset;
   float zoom_level;
} pc;

layout(location = 0) out vec2 out_uv;

void main() {
   gl_Position = vec4((in_position - pc.pos_offset) * pc.zoom_level, 0, 1);

   out_uv = in_uv;
}