#include <cuda_runtime.h>
#include <optix.h>
#include <optix_device.h>
#include <vector_functions.h>
#include <curand_kernel.h>

struct Params
{
    uchar4* image;
    unsigned int image_width;
    unsigned int image_height;
    OptixTraversableHandle handle;
};

struct RayGenData
{
    float3 pixel_delta;
    float3 pixel_00_loc;
    float3 cam_eye;
};

struct MissData
{
    unsigned int r, g, b;
};

struct HitGroupData
{
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
    __global__ void __raygen__rg()
    {
        uint3 launch_index = optixGetLaunchIndex();
        RayGenData* rtData = (RayGenData*)optixGetSbtDataPointer();

        float3 pixel_center = make_float3(rtData->pixel_00_loc.x + (launch_index.x * rtData->pixel_delta.x),
            rtData->pixel_00_loc.y + (launch_index.y * rtData->pixel_delta.y),
            rtData->pixel_00_loc.z);

        unsigned int p0 = 0; //(unsigned int)(128);
        unsigned int p1 = 0; //(unsigned int)(128);
        unsigned int p2 = 0; //(unsigned int)(128);

        //curandState state;
        //curand_init(1234, launch_index.x, 0, &state);


        for (uint8_t r = 0; r < 4; ++r)
        {
            unsigned int pr0 = 0;
            unsigned int pr1 = 0;
            unsigned int pr2 = 0;

            float3 ray_dir = {
                pixel_center.x - rtData->cam_eye.x,
                pixel_center.y - rtData->cam_eye.y,
                pixel_center.z - rtData->cam_eye.z };

            //float offset_x = curand_uniform(&state);
            //float offset_y = curand_uniform(&state);

            float3_normalize(&ray_dir);
            optixTrace(params.handle, pixel_center, ray_dir, 0.01, 1e16f, 0, 0xFF, 0, 0, 0, 0, pr0, pr1, pr2);

            p0 += pr0;
            p1 += pr1;
            p2 += pr2;
        }

        p0 /= 4;
        p1 /= 4;
        p2 /= 4;

        params.image[launch_index.y * params.image_width + launch_index.x] = make_uchar4(
            p0,
            p1,
            p2,
            255);
    }

    __global__ void __miss__ms()
    {
        MissData* rt_data = reinterpret_cast<MissData*>(optixGetSbtDataPointer());
        optixSetPayload_0(64);
        optixSetPayload_1(64);
        optixSetPayload_2(64);
    }

    __global__ void __closesthit__ch()
    {
        float t_hit = optixGetRayTmax();

        const float3 ray_org = optixGetWorldRayOrigin();
        const float3 ray_dir = optixGetWorldRayDirection();
        float3 ray_hit = float3{ ray_dir.x * t_hit, ray_dir.y * t_hit, ray_dir.z * t_hit };
        ray_hit.x += ray_org.x;
        ray_hit.y += ray_org.y;
        ray_hit.z += ray_org.z;

        const unsigned int prim_idx = optixGetPrimitiveIndex();
        const OptixTraversableHandle gas = optixGetGASTraversableHandle();
        const unsigned int sbtGASIndex = optixGetSbtGASIndex();

        float4 q;
        optixGetSphereData(gas, prim_idx, sbtGASIndex, 0.f, &q);

        float3 normal = { ray_hit.x - q.x, ray_hit.y - q.y, ray_hit.z - q.z };
        float3_normalize(&normal);

        optixSetPayload_0(normal.z * 255);
        optixSetPayload_1(normal.y * 255);
        optixSetPayload_2(normal.x * 255);
    }
}