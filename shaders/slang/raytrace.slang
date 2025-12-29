struct CamMatrices
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
ConstantBuffer<CamMatrices> cam_matrices;

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
    uint32_t material_index;
};

[[vk::shader_record]] ConstantBuffer<CH_SBT_Record> ch_sbt;

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
    float4 target = mul(cam_matrices.proj_inverse, float4(d.x, d.y, 1, 1));

    RayDesc ray_desc;
    ray_desc.Origin = mul(cam_matrices.view_inverse, float4(0, 0, 0, 1)).xyz;
    ray_desc.Direction = mul(cam_matrices.view_inverse, float4(normalize(target.xyz), 0)).xyz;
    ray_desc.TMin = 0.001f;
    ray_desc.TMax = 1000.f;

    Payload payload;

    TraceRay(tlas, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray_desc, payload);

    final_target[launch_id.xy] = float4(payload.hit_value, 1);
}

[shader("miss")]
void miss(inout Payload p)
{
    uint3 launch_id = DispatchRaysIndex();
    uint3 launch_size = DispatchRaysDimensions();

    uint flat_index = launch_id.y * launch_size.x + launch_id.x;
    uint4 z = rand_states[flat_index];

    // p.hit_value = float3(HybridTaus(z), HybridTaus(z), HybridTaus(z));
    p.hit_value = float3(0, 0, 0.2f);
    rand_states[flat_index] = z;
}

struct Attributes
{
    float2 bary;
};

[shader("closesthit")]
void closesthit(inout Payload p, in Attributes attribs)
{
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    p.hit_value = barycentricCoords;
}
