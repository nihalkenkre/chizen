#include "common.cu.h"


extern "C" __constant__ launch_params lp;

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

	return ray_cu_create(org, dir);
}


extern "C"  __global__ void __raygen__rg()
{
	uint3 launch_index = optixGetLaunchIndex();
	ray_gen_record_data* rg_data = (ray_gen_record_data*)optixGetSbtDataPointer();

	curandState rand_state = { 0 };
	curand_init(1237, launch_index.y * lp.render_width + launch_index.x, 0, &rand_state);

	ray_cu r = generate_ray(rand_state, launch_index.x, launch_index.y, rg_data->pixel_00_loc, rg_data->pixel_delta_u, rg_data->pixel_delta_v, rg_data->org);

	unsigned p0 = 0, p1 = 0, p2 = 0, p3 = 0;

	optixTrace(lp.handle, r.org, r.dir, 0.f, 1000.f, 0.f, 0xFF, 0, 0, 0, 0, p0, p1, p2, p3);

	size_t pixel_idx = (launch_index.y * lp.render_width * 4) + (launch_index.x * 4);
	lp.pixels[pixel_idx] = p0;
	lp.pixels[pixel_idx + 1] = p1;
	lp.pixels[pixel_idx + 2] = p2;
	lp.pixels[pixel_idx + 3] = p3;
}

extern "C" __global__ void __closesthit__ch()
{
	float2 bary_coords = optixGetTriangleBarycentrics();
	uint3 launch_index = optixGetLaunchIndex();

	optixSetPayload_0(bary_coords.x * 255);
	optixSetPayload_1(bary_coords.y * 255);
	optixSetPayload_2((1 - bary_coords.x - bary_coords.y) * 255);
	optixSetPayload_3(255);
}

extern "C" __global__ void __miss__ms()
{
	miss_record_data* ms_data = (miss_record_data*)optixGetSbtDataPointer();

	optixSetPayload_0(0);
	optixSetPayload_1(0);
	optixSetPayload_2(0);
	optixSetPayload_3(0);
}