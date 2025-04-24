#version 460

// layout(location = 0) in vec4 in_color;

// layout(set = 1, binding = 5) uniform sampler2D s;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(1, 0, 0, 1);//in_color;
}