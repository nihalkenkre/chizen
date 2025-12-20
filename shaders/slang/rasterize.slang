struct VSInput
{
    [[vk::location(0)]]
    float3 pos;
    [[vk::location(1)]]
    float3 nrm;
    [[vk::location(2)]]
    float2 uv;
}

struct VSOutput
{
    float4 pos : SV_Position;
    [[vk::location(0)]]
    float2 uv;
    [[vk::location(1)]]
    float3 nrm;
}

struct ViewProj
{
    float4x4 view;
    float4x4 proj;
}

struct Model
{
    float4x4 model;
}

[[vk::binding(0, 0)]]
ConstantBuffer<ViewProj> view_proj;
[[vk::binding(0, 1)]]
ConstantBuffer<Model> model;

[shader("vertex")]
VSOutput vertex_main(VSInput in)
{
    VSOutput out = {
        mul(view_proj.proj, mul(view_proj.view, mul(model.model, float4(in.pos, 1)))),
        in.uv,
        in.nrm,
    };

    return out;
}

[[vk::binding(0, 2)]]
Sampler2D base_color_factor;
[[vk::binding(1, 2)]]
Sampler2D base_color_texture;
[[vk::binding(2, 2)]]
Sampler2D normal_texture;

struct FSOutput
{
    float4 color;
}

[shader("fragment")]
FSOutput fragment_main(VSOutput in)
{
    float4 base_color = base_color_texture.Sample(in.uv);

    if (all(base_color == 0.f))
    {
        base_color = base_color_factor.Sample(in.uv);
    }
    else
    {
        base_color *= base_color_factor.Sample(in.uv);
    }

    FSOutput fs_output = {base_color};
    return fs_output;
}
