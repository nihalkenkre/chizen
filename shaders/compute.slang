[[vk::binding(0, 0)]]
RWTexture2D<float4> image;

[shader("compute")]
[numthreads(1, 1, 1)]
void compute_main(uint3 thread_id: SV_DispatchThreadID)
{
    uint width = 0, height = 0;
    image.GetDimensions(width, height);

    image[thread_id.xy] = float4((float)thread_id.x / (float)width, (float)thread_id.y / (float)height, 0, 1);
}
