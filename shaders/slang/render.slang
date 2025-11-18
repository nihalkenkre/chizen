[[vk::binding(0, 0)]]
RWTexture2D<float4> accum_target;
[[vk::binding(1, 0)]]
RWTexture2D<float4> final_render;
[[vk::binding(2, 0)]]
RWStructuredBuffer<uint4> rand_states;

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

[shader("compute")]
[numthreads(32, 32, 1)]
void compute_main(uint3 thread_id: SV_DispatchThreadID, uniform int2 dispatch_size, uniform uint current_sample)
{
    if (thread_id.x > dispatch_size.x || thread_id.y > dispatch_size.y)
        return;

    uint flat_index = thread_id.z * (dispatch_size.x * dispatch_size.y) + thread_id.y * dispatch_size.x + thread_id.x;
    uint4 z = rand_states[flat_index];

    float4 out_color = float4(
        HybridTaus(z),
        HybridTaus(z),
        HybridTaus(z),
        1);

    accum_target[thread_id.xy] += out_color;
    final_render[thread_id.xy] = accum_target[thread_id.xy] / current_sample;
    rand_states[flat_index] = z;
}
