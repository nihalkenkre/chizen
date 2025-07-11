#include <stdint.h>

#include <curand_kernel.h>
#include <cuda_runtime.h>

#include <optix.h>
#include <optix_stubs.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <optix_stack_size.h>
#include <optix_function_table_definition.h>

#include "../src/triangle.h"

typedef struct launch_params
{
    uint8_t *pixels;
    size_t render_width;
    size_t render_height;
    OptixTraversableHandle handle;
} launch_params;

typedef struct ray_gen_record_data
{
    float3 pixel_00_loc;
    float3 pixel_delta_u;
    float3 pixel_delta_v;
    float3 org;
} ray_gen_record_data;

typedef struct ray_gen_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT)
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    ray_gen_record_data data;
} ray_gen_record;

typedef struct miss_record_data
{
    float3 DUMMY;
} miss_record_data;

typedef struct miss_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT)
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    miss_record_data* data;
} miss_record;

typedef struct ray_cu
{
	float3 org;
	float3 dir;
	float3 inv_dir;
	uint3 sign;
} ray_cu;

__device__ static ray_cu ray_cu_create(float3 org, float3 dir)
{
	ray_cu r = { 0 };
	r.org = org;
	r.dir = dir;
	r.inv_dir = { 1.f / dir.x, 1.f / dir.y, 1.f / dir.z };
	r.sign = { r.inv_dir.x < 0.f, r.inv_dir.y < 0.f, r.inv_dir.z < 0.f };

	return r;
}

__device__ float3 static float3_add(float3 a, float3 b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

__device__ float3 static float3_scale(float3 v, float s)
{
	return { v.x * s, v.y * s, v.z * s };
}

__device__ float static float3_dot(float3 a, float3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

__device__ float3 static float3_sub(float3 a, float3 b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

__device__ float3 static float3_normalize(float3 v)
{
	float mag = sqrtf(float3_dot(v, v));

	return float3_scale(v, 1.f / mag);
}

__device__ static float3 ray_cu_pt_at(ray_cu r, const float t)
{
	float3 pt = float3_scale(r.dir, t);
	return float3_add(r.org, pt);
}

__device__ void static hit_triangle(triangle tri, ray_cu ray)
{

}

__device__ float static hit_sphere(float3 center, const float radius, ray_cu ray)
{
	float3 oc = float3_sub(center, ray.org);
	float a = float3_dot(ray.dir, ray.dir);
	float b = -2.f * float3_dot(ray.dir, oc);
	float c = float3_dot(oc, oc) - radius * radius;
	float discrim = b * b - 4.f * a * c;

	if (discrim <= 0.f)
		return discrim;
	else
		return (-b - sqrtf(discrim)) / (2.f * a);
}

__device__ ray_cu static generate_ray(curandState rand_state, const size_t x, const size_t y, float3 pixel_00_loc, float3 pixel_delta_u, float3 pixel_delta_v, float3 org)
{
	float3 offset = { curand_uniform(&rand_state), curand_uniform(&rand_state), curand_uniform(&rand_state) };

	float3 pixel_delta_u_x = float3_scale(pixel_delta_u, (float)x);
	float3 pixel_delta_v_y = float3_scale(pixel_delta_v, (float)y);

	float3 pixel_delta_u_offset = float3_scale(pixel_delta_u, offset.x);
	float3 pixel_delta_v_offset = float3_scale(pixel_delta_v, offset.y);

	float3 pixel_center = float3_add(pixel_00_loc, pixel_delta_u_x);
	pixel_center = float3_add(pixel_center, pixel_delta_v_y);
	pixel_center = float3_add(pixel_center, pixel_delta_u_offset);
	pixel_center = float3_add(pixel_center, pixel_delta_v_offset);

	float3 dir = float3_normalize(float3_sub(pixel_center, org));

	//if ((x == 640 && y == 360) ||
	//	(x == 0 && y == 0) ||
	//	(x == 1219 && y == 729)
	//	)
	//{
	//	printf("pixel center: ");
	//	printf("%f %f %f\n", pixel_center.x, pixel_center.y, pixel_center.z);
	//	printf("dir: ");
	//	printf("%f %f %f\n", dir.x, dir.y, dir.z);
	//}

	return ray_cu_create(org, dir);
}

