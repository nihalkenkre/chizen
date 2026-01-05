struct CamInfo
{
    float4x4 view_inverse;
    float4x4 proj_inverse;
}

struct Material
{
    float4 base_color_factor;
    int4 base_color_index;
};

struct PushConstants
{
    uint current_sample;
}

[[vk::binding(0, 0)]]
ConstantBuffer<CamInfo> cam_info;

[[vk::binding(0, 1)]]
RWTexture2D<float4> accum_target;
[[vk::binding(1, 1)]]
RWTexture2D<float4> final_target;
[[vk::binding(2, 1)]]
RWStructuredBuffer<uint4> rand_states;

[[vk::binding(3, 1)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(4, 1)]]
StructuredBuffer<Material> materials;
[[vk::binding(5, 1)]]
Sampler2D textures[];

[[vk::push_constant]]
PushConstants pc;

struct Payload
{
    float3 hit_value;
};

struct CH_SBT_Record
{
    float3* normals;
    float2* uv0s;
    uint3* indices;
    uint64_t material_index;
};

[[vk::shader_record]]
ConstantBuffer<CH_SBT_Record> ch_sbt;

uint TausStep(inout uint z, int S1, int S2, int S3, uint M)
{
    uint b = (((z << S1) ^ z) >> S2);
    return z = (((z & M) << S3) ^ b);
}

uint LCGStep(inout uint z, uint A, uint C)
{
    return z = (A * z + C);
}

float HybridTaus(inout uint4 z)
{
    return 2.3283064365387e-10 * (TausStep(z.x, 13, 19, 12, 4294967294) ^
                                  TausStep(z.y, 2, 25, 4, 4294967288) ^
                                  TausStep(z.z, 3, 11, 17, 4294967280) ^
                                  LCGStep(z.w, 1664525, 1013904223));
}

[shader("raygeneration")]
void raygen()
{
    uint3 launch_id = DispatchRaysIndex();
    uint3 launch_size = DispatchRaysDimensions();

    const float2 pixel_center = float2(launch_id.xy) + float2(0.5, 0.5);
    const float2 in_uv = pixel_center / float2(launch_size.xy);
    float2 d = in_uv * 2.0 - 1.0;
    float4 target = mul(cam_info.proj_inverse, float4(d.x, d.y, 1, 1));

    RayDesc ray_desc;
    ray_desc.Origin = mul(cam_info.view_inverse, float4(0, 0, 0, 1)).xyz;
    ray_desc.Direction = mul(cam_info.view_inverse, float4(normalize(target.xyz), 0)).xyz;
    ray_desc.TMin = 0.01f;
    ray_desc.TMax = 1000.f;

    Payload payload;

    TraceRay(tlas, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray_desc, payload);

    accum_target[launch_id.xy] += float4(payload.hit_value, 1);
    final_target[launch_id.xy] = accum_target[launch_id.xy] / pc.current_sample;
}

[shader("miss")]
void miss(inout Payload p)
{
    uint3 launch_id = DispatchRaysIndex();
    uint3 launch_size = DispatchRaysDimensions();

    uint flat_index = launch_id.y * launch_size.x + launch_id.x;
    uint4 z = rand_states[flat_index];

    p.hit_value = float3(HybridTaus(z), HybridTaus(z), HybridTaus(z));
    // p.hit_value = float3(0.2f, 0.2f, 0.2f);
    rand_states[flat_index] = z;
}

struct Attributes
{
    float2 bary;
};

[shader("closesthit")]
void closesthit(inout Payload p, in Attributes attribs)
{
    const float3 bary_coords = float3(attribs.bary.x, attribs.bary.y, 1.0f - attribs.bary.x - attribs.bary.y);

    uint3 index_triplet = *(ch_sbt.indices + PrimitiveIndex());

    float2 uv = ch_sbt.uv0s[index_triplet.x] * bary_coords.z +
                ch_sbt.uv0s[index_triplet.y] * bary_coords.x +
                ch_sbt.uv0s[index_triplet.z] * bary_coords.y;
    float3 normal = ch_sbt.normals[index_triplet.x] * bary_coords.z +
                    ch_sbt.normals[index_triplet.y] * bary_coords.x +
                    ch_sbt.normals[index_triplet.z] * bary_coords.y;

    float4 base_color = materials[ch_sbt.material_index].base_color_factor;
    if (materials[ch_sbt.material_index].base_color_index.x >= 0)
    {
        base_color *= textures[NonUniformResourceIndex(materials[ch_sbt.material_index].base_color_index.x)].SampleLevel(uv, 0);
    }

    p.hit_value = base_color.xyz;
}
