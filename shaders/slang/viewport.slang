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
        in.nrm,
        in.uv,
    };

    return out;
}

struct Material
{
    float4 base_color_factor;
    int4 base_color_index;
};

struct PushConstants
{
    int material_index;
}

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

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
    float4 base_color = float4(1.f);
    if (pc.material_index >= 0)
    {
        if (materials[pc.material_index].base_color_index.x >= 0)
        {
            base_color *= textures[NonUniformResourceIndex(materials[pc.material_index].base_color_index.x)].Sample(in.uv) * materials[pc.material_index].base_color_factor;
        }
        else
        {
            base_color *= materials[pc.material_index].base_color_factor;
        }
    }

    FSOutput fs_output = { base_color };
    return fs_output;
}
