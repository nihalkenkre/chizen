StructuredBuffer<float> buffer0;
StructuredBuffer<float> buffer1;
RWStructuredBuffer<float> result;

[shader("compute")]
[numthreads(1, 1, 1)]
void compute_main(uint3 thread_id: SV_DispatchThreadID)
{
    uint index = thread_id.x;
    result[index] = buffer0[index] + buffer1[index];
}