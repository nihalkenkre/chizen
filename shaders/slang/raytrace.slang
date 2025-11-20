RWTexture2D<float4> accum_target;
RWTexture2D<float4> final_target;
RWStructuredBuffer<uint4> rand_states;

struct PushConstants
{
    uint current_sample;
}

[[vk::push_constant]] PushConstants pc;

struct Payload
{
    float2 bary;
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

    accum_target[launch_id.xy] += out_color;
    final_target[launch_id.xy] = accum_target[launch_id.xy] / pc.current_sample;
    rand_states[flat_index] = z;
}

[shader("miss")]
void miss(inout Payload p)
{
}

[shader("closesthit")]
void closesthit(inout Payload p)
{
}
