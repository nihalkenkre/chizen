struct VSInput
{
    [[vk::location(0)]]
    float3 pos;
    [[vk::location(1)]]
    float2 uv;
    [[vk::location(2)]]
    float3 nrm;
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

struct FSOutput
{
    float4 color;
}

[shader("fragment")]
FSOutput fragment_main(VSOutput in)
{
    FSOutput fs_output = { float4(in.uv, 0, 1) };
    return fs_output;
}
