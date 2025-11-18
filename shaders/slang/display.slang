struct VSInput
{
    [[vk::location(0)]]
    float2 pos;
    [[vk::location(1)]]
    float2 uv;
}

struct VSOutput
{
    float4 pos : SV_Position;
    [[vk_location(0)]]
    float2 uv;
}

[shader("vertex")]
VSOutput vertex_main(VSInput in, uniform float2 pos_offset, uniform float zoom_level)
{
    VSOutput out;
    out.pos = float4((in.pos * zoom_level) - pos_offset, 0, 1);
    out.uv = in.uv;

    return out;
}

[[vk::binding(0, 0)]]
Sampler2D final_render;

[shader("fragment")]
float4 fragment_main(VSOutput in)
{
    return final_render.Sample(in.uv);
}
