RWTexture2D<float4> accum_target;
RWTexture2D<float4> final_target;

[shader("raygeneration")]
void raygen(uniform uint current_sample)
{
    uint3 launch_id = DispatchRaysIndex();
    uint3 launch_size = DispatchRaysDimensions();

    final_target[launch_id.xy] = float4(launch_id / launch_size, 1);
}
