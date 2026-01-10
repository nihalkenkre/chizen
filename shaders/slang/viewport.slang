struct VSInput
{
    float3 pos;
    float3 nrm;
    float2 uv;
}

struct VSOutput
{
    float4 pos : SV_Position;
    float3 nrm;
    float2 uv;
}

struct ViewProj
{
    float4x4 view_proj;
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
        mul(view_proj.view_proj, mul(model.model, float4(in.pos, 1))),
        in.nrm,
        in.uv,
    };

    return out;
}

import "common.slang";

struct PushConstants
{
    uint material_index;
    uint is_cpu_shading;
}

[[vk::push_constant]]
PushConstants pc;

[[vk::binding(0, 2)]]
StructuredBuffer<Material> materials;
[[vk::binding(1, 2)]]
Sampler2D textures[];

struct FSOutput
{
    float4 color;
}

[shader("fragment")]
FSOutput fragment_main(VSOutput in)
{
    float4 base_color = materials[pc.material_index].base_color_factor;

    if (pc.is_cpu_shading == 0)
    {
        if (materials[pc.material_index].base_normal_metalrough_index.x >= 0)
        {
            base_color *= textures[NonUniformResourceIndex(materials[pc.material_index].base_normal_metalrough_index.x)].Sample(in.uv);
        }
    }

    FSOutput fs_output = { base_color };
    return fs_output;
}
