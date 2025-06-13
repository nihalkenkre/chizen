#include <cuda_runtime.h>
#include <optix.h>
#include <optix_device.h>
#include <vector_functions.h>

struct Params
{
    uchar4* image;
    unsigned int image_width;
    unsigned int image_height;
};

struct RayGenData
{
    float3 pixel_delta;
    float3 pixel_00_loc;
    float3 cam_eye;
};

__constant__ Params params;

__device__ void float3_normalize(float3* v)
{
    float mag = sqrtf((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
    v->x = fdividef(v->x, mag);
    v->y = fdividef(v->y, mag);
    v->z = fdividef(v->z, mag);
}

extern "C"
{
    __global__ void __raygen__draw_solid_color()
    {
        uint3 launch_index = optixGetLaunchIndex();
        RayGenData* rtData = (RayGenData*)optixGetSbtDataPointer();

        float3 pixel_center = make_float3(rtData->pixel_00_loc.x + (launch_index.x * rtData->pixel_delta.x),
            rtData->pixel_00_loc.y + (launch_index.y * rtData->pixel_delta.y),
            rtData->pixel_00_loc.z
        );

        float3 ray_dir = {
            pixel_center.x - rtData->cam_eye.x,
            pixel_center.y - rtData->cam_eye.y,
            pixel_center.z - rtData->cam_eye.z
        };

        float3_normalize(&ray_dir);
        ray_dir.y = (ray_dir.y + 1) * 0.5;

        // make_uchar4(launch_index.x % params.image_width, launch_index.y % params.image_height, 0, 255);
        params.image[launch_index.y * params.image_width + launch_index.x] = make_uchar4(
            0,//(unsigned char)(((float)(launch_index.x) / params.image_width) * 255),
            (unsigned char)(ray_dir.y * 255),
            0,//(unsigned char)(((float)(launch_index.y) / params.image_height) * 255),
            255
        );
    }
}