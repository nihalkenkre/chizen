#include <optix.h>
#include <optix_device.h>
#include <vector_functions.h>
#include <cuda_runtime.h>
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
    unsigned int num_aa_samples;
    curandState* rand_states;
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

        unsigned int p0 = 0;
        unsigned int p1 = 0;
        unsigned int p2 = 0;
        unsigned int p3 = 0;

        curand_init(launch_index.x + launch_index.y, launch_index.x + launch_index.y, 0, &rtData->rand_states[threadIdx.x + blockIdx.x * blockDim.x]);

        for (uint8_t r = 0; r < rtData->num_aa_samples; ++r)
        {
            unsigned int pr0 = 0;
            unsigned int pr1 = 0;
            unsigned int pr2 = 0;
            unsigned int pr3 = 0;

            float offset_x = curand_uniform(&rtData->rand_states[threadIdx.x + blockIdx.x * blockDim.x]) * rtData->pixel_delta.x;
            float offset_y = curand_uniform(&rtData->rand_states[threadIdx.x + blockIdx.x * blockDim.x]) * rtData->pixel_delta.y;

            float3 ray_dir = {
                pixel_center.x + offset_x - rtData->cam_eye.x,
                pixel_center.y + offset_y - rtData->cam_eye.y,
                pixel_center.z - rtData->cam_eye.z,
            };

            float3_normalize(&ray_dir);
            optixTrace(params.handle, pixel_center, ray_dir, 0.01, 1e16f, 0, 0xFF, 0, 0, 0, 0, pr0, pr1, pr2, pr3);

            p0 += pr0;
            p1 += pr1;
            p2 += pr2;
            p3 += pr3;
        }

        p0 /= rtData->num_aa_samples;
        p1 /= rtData->num_aa_samples;
        p2 /= rtData->num_aa_samples;
        p3 /= rtData->num_aa_samples;

        params.image[launch_index.y * params.image_width + launch_index.x] = make_uchar4(
            p0,
            p1,
            p2,
            p3);
    }

    __global__ void __miss__ms()
    {
        MissData* rt_data = reinterpret_cast<MissData*>(optixGetSbtDataPointer());

        float3 ray_dir = optixGetWorldRayDirection();
        float3_normalize(&ray_dir);
        ray_dir.y += 1;
        ray_dir.y *= 0.5;
        ray_dir.y = 1 - ray_dir.y;
        optixSetPayload_0(0);
        optixSetPayload_1(ray_dir.y * 255);
        optixSetPayload_2(0);
        optixSetPayload_3(255);
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

        optixSetPayload_0(normal.x * 255);
        optixSetPayload_1(normal.y * 255);
        optixSetPayload_2(normal.z * 255);
        optixSetPayload_3(255);
    }
}