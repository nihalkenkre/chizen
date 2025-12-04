RWTexture2D<float4> accum_target;
RWTexture2D<float4> final_target;
RWStructuredBuffer<uint4> rand_states;

struct UniformBuffer
{
    float4x4 view_inverse;
    float4x4 proj_inverse;
}

ConstantBuffer<UniformBuffer> uniform_buffer;
RaytracingAccelerationStructure tlas;

struct PushConstants
{
    uint current_sample;
}

[[vk::push_constant]]
PushConstants pc;

struct Payload
{
    float3 hit_value;
};

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

    uint flat_index = launch_id.z * (launch_size.x * launch_size.y) + launch_id.y * launch_size.x + launch_id.x;
    uint4 z = rand_states[flat_index];

    float4 out_color = float4(
        HybridTaus(z),
        HybridTaus(z),
        HybridTaus(z),
        1);

    const float2 pixelCenter = float2(launch_id.xy) + float2(0.5, 0.5);
    const float2 inUV = pixelCenter / float2(launch_size.xy);
    float2 d = inUV * 2.0 - 1.0;
    float4 target = mul(uniform_buffer.proj_inverse, float4(d.x, d.y, 1, 1));

    RayDesc ray_desc;
    ray_desc.Origin = mul(uniform_buffer.view_inverse, float4(0, 0, 0, 1)).xyz;
    ray_desc.Direction = mul(uniform_buffer.view_inverse, float4(normalize(target.xyz), 0)).xyz;
    ray_desc.TMin = 0.001f;
    ray_desc.TMax = 1000.f;

    Payload payload;
    payload.hit_value = ray_desc.Direction;

    TraceRay(tlas, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray_desc, payload);

    accum_target[launch_id.xy] += float4(payload.hit_value, 1);
    final_target[launch_id.xy] = accum_target[launch_id.xy] / pc.current_sample;
    rand_states[flat_index] = z;
}

[shader("miss")]
void miss(inout Payload p)
{
    p.hit_value = float3(0.1, 0.2, 0.3);
}

[shader("closesthit")]
void closesthit(inout Payload p)
{
    p.hit_value = float3(0.5, 0.0, 0.0);
}
