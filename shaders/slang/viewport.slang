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
    uint material_index;
}

struct ViewProj
{
    float4x4 view_proj;
}

struct DrawElement
{
    uint model_matrix_index;
    uint material_index;
};

struct PushConstants
{
    float4x4 *view_proj;
    DrawElement *draw_elements;
    Material *materials;
    float4x4 *model_matrices;
    Light *lights;
    uint lights_count;
    uint is_cpu_shading;
};

[[vk::push_constant]]
PushConstants pc;

// [[vk::binding(0, 0)]]
// ConstantBuffer<ViewProj> view_proj;

[shader("vertex")]
VSOutput vertex(VSInput in, uint draw_id: SV_DrawIndex)
{
    float3 t = (in.tngt * in.tngt.w).xyz;
    float3 n = in.nrm;
    t = normalize(t - dot(t, n) * n);
    float3 b = cross(n, t);

    float4x4 model_matrix = pc.model_matrices[pc.draw_elements[draw_id].model_matrix_index];

    t = normalize(mul(model_matrix, float4(t, 0)).xyz);
    b = normalize(mul(model_matrix, float4(b, 0)).xyz);
    n = normalize(mul(model_matrix, float4(n, 0)).xyz);

    VSOutput out = {
        mul(*pc.view_proj, mul(model_matrix, float4(in.pos, 1))),
        t,
        b,
        n,
        in.uv,
        pc.draw_elements[draw_id].material_index,
    };

    return out;
}

import "utils.slang";

[[vk::binding(0, 0)]]
Sampler2D textures[];

struct FSOutput
{
    float4 color;
}

[shader("fragment")]
FSOutput fragment(VSOutput in)
{
    float4 final_color = float4(0, 0, 0, 1.f);
    Material material = pc.materials[in.material_index];
    float4 base_color = material.base_color_factor;

    float3 n = in.n;
    if (pc.is_cpu_shading == 0)
    {
        if (material.base_normal_metalrough_index.x >= 0)
        {
            base_color *= textures[NonUniformResourceIndex(material.base_normal_metalrough_index.x)].Sample(in.uv);
        }

        if (material.base_normal_metalrough_index.y >= 0)
        {
            n = mul(
                transpose(float3x3(in.t, in.b, in.n)),
                textures[NonUniformResourceIndex(material.base_normal_metalrough_index.y)].Sample(in.uv).xyz * 2 - 1);
        }
    }

    for (uint l = 0; l < pc.lights_count; ++l)
    {
        final_color.xyz += (base_color.xyz * (dot(-pc.lights[l].direction, n)));
    }

    FSOutput fs_output = { final_color };
    return fs_output;
}
