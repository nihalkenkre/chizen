#version 460

layout(location = 0) in vec2 in_uv;

// prim level
layout(set = 1, binding = 1) uniform sampler2D base_color;
layout(set = 1, binding = 2) uniform sampler2D metal_rough;
layout(set = 1, binding = 3) uniform sampler2D cc;
layout(set = 1, binding = 4) uniform sampler2D cc_rough;
layout(set = 1, binding = 5) uniform sampler2D cc_desc;
layout(set = 1, binding = 6) uniform Factors {
    float[4] base_color_factor;
    float metal_factor;
    float rough_factor;

    float cc_factor;
    float cc_rough_factor;
} factors;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(base_color, in_uv);// vec4(in_uv, 1, 1);
}