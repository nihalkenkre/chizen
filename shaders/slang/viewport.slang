struct VSInput
{
    float3 pos;
    float4 tngt;
    float3 nrm;
    float2 uv;
}

struct VSOutput
{
    float4 pos : SV_Position;
    float3 t;
    float3 b;
    float3 n;
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
VSOutput vertex(VSInput in)
{
    float3 t = (in.tngt * in.tngt.w).xyz;
    float3 n = in.nrm;
    t = normalize(t - dot(t, n) * n);
    float3 b = cross(n, t);

    t = normalize(mul(model.model, float4(t, 0)).xyz);
    b = normalize(mul(model.model, float4(b, 0)).xyz);
    n = normalize(mul(model.model, float4(n, 0)).xyz);

    VSOutput out = {
        mul(view_proj.view_proj, mul(model.model, float4(in.pos, 1))),
        t,
        b,
        n,
        in.uv,
    };

    return out;
}

import "utils.slang";

struct PushConstants
{
    Light *lights;
    uint lights_count;
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
FSOutput fragment(VSOutput in)
{
    float4 final_color = float4(0, 0, 0, 1.f);
    float4 base_color = materials[pc.material_index].base_color_factor;

    if (pc.is_cpu_shading == 0)
    {
        if (materials[pc.material_index].base_normal_metalrough_index.x >= 0)
        {
            base_color *= textures[NonUniformResourceIndex(materials[pc.material_index].base_normal_metalrough_index.x)].Sample(in.uv);
        }
    }

    for (uint l = 0; l < pc.lights_count; ++l)
    {
        final_color.xyz += (base_color.xyz * (dot(-pc.lights[l].direction, in.n)));
    }

    FSOutput fs_output = { final_color };
    return fs_output;
}
