#version 460

layout(location = 0) in vec2 in_uv;

// material level
layout(set = 1, binding = 0) uniform sampler2D base_color;
layout(set = 1, binding = 1) uniform sampler2D metal_rough;
layout(set = 1, binding = 2) uniform Factors {
    float[4] base_color_factor;
    float metal_factor;
    float rough_factor;
} factors;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(base_color, in_uv);// vec4(in_uv, 1, 1);
}