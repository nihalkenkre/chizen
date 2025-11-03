#version 460

layout(location = 0) in vec2 in_uv;

layout(set=0, binding=0) uniform sampler2D render_target;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(render_target, in_uv);
}