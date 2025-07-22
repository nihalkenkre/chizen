#include "common.cu.h"
#include "../utils.h"
#include <sutil/vec_math.h>

typedef struct ray_payload
{
	float4 p;
} ray_payload;

extern "C" __constant__ launch_params lp;

__device__ static ray ray_create(float3 org, float3 dir)
{
	ray r = { 0 };
	r.org = org;
	r.dir = dir;
	r.inv_dir = { 1.f / dir.x, 1.f / dir.y, 1.f / dir.z };
	r.sign = { r.inv_dir.x < 0.f, r.inv_dir.y < 0.f, r.inv_dir.z < 0.f };

	return r;
}

__device__ static float3 float3_add(float3 a, float3 b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

__device__ static float3 float3_scale(float3 v, float s)
{
	return { v.x * s, v.y * s, v.z * s };
}

__device__ static float float3_dot(float3 a, float3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

__device__ static float3 float3_sub(float3 a, float3 b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

__device__ static float3 float3_normalize(float3 v)
{
	float mag = sqrtf(float3_dot(v, v));

	return float3_scale(v, 1.f / mag);
}

__device__ static float3 ray_pt_at(ray r, const float t)
{
	float3 pt = float3_scale(r.dir, t);
	return float3_add(r.org, pt);
}

__device__ static float4 color_blend(float4 src, float4 dst)
{
	return float4{
		(src.x * src.w) + (dst.x * (1 - src.w)),
		(src.y * src.w) + (dst.y * (1 - src.w)),
		(src.z * src.w) + (dst.z * (1 - src.w)),
		src.w
	};
}

__device__ static ray generate_ray(float2 offset, const size_t x, const size_t y, float3 pixel_00_loc, float3 pixel_delta_u, float3 pixel_delta_v, float3 org)
{
	float3 pixel_delta_u_x = float3_scale(pixel_delta_u, (float)x);
	float3 pixel_delta_v_y = float3_scale(pixel_delta_v, (float)y);

	float3 pixel_delta_u_offset = float3_scale(pixel_delta_u, offset.x);
	float3 pixel_delta_v_offset = float3_scale(pixel_delta_v, offset.y);

	float3 pixel_center = float3_add(pixel_00_loc, pixel_delta_u_x);
	pixel_center = float3_add(pixel_center, pixel_delta_v_y);
	pixel_center = float3_add(pixel_center, pixel_delta_u_offset);
	pixel_center = float3_add(pixel_center, pixel_delta_v_offset);

	float3 dir = float3_normalize(float3_sub(pixel_center, org));

	return ray_create(org, dir);
}

extern "C"  __global__ void __raygen__rg()
{
	uint3 launch_index = optixGetLaunchIndex();
	ray_gen_record_data* rg_data = (ray_gen_record_data*)optixGetSbtDataPointer();

	//float n0 = 0, n1 = 0, n2 = 0, n3 = 0;
	//float uv0 = 0, uv1 = 0, uv2 = 0, uv3 = 0;
	unsigned int r_idx = threadIdx.x + blockIdx.x * blockDim.x;

	curand_init(launch_index.x + launch_index.y, launch_index.x + launch_index.y, 0, &rg_data->states[r_idx]);

	for (size_t s = 0; s < rg_data->num_samples; ++s) {
		float2 offset = { curand_uniform(&rg_data->states[r_idx]), curand_uniform(&rg_data->states[r_idx]) };
		ray r = generate_ray(offset, launch_index.x, launch_index.y, rg_data->pixel_00_loc, rg_data->pixel_delta_u, rg_data->pixel_delta_v, rg_data->org);

		optixTrace(lp.handle, r.org, r.dir, 0.f, 1000.f, 0.f, 0xFF, 0, 0, 0, 0);
	}

	size_t pixel_idx = (launch_index.y * lp.render_width * 4) + (launch_index.x * 4);

	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		lp.passes[p].pixels[pixel_idx] /= rg_data->num_samples;
		lp.passes[p].pixels[pixel_idx + 1] /= rg_data->num_samples;
		lp.passes[p].pixels[pixel_idx + 2] /= rg_data->num_samples;
		lp.passes[p].pixels[pixel_idx + 3] /= rg_data->num_samples;
	}
}

extern "C" __global__ void __closesthit__ch()
{
	float2 tmp_bary_coords = optixGetTriangleBarycentrics();
	float3 bary_coords = {
		tmp_bary_coords.x,
		tmp_bary_coords.y,
		1.f - tmp_bary_coords.x - tmp_bary_coords.y,
	};
	uint3 launch_index = optixGetLaunchIndex();

	unsigned int primitive_idx = optixGetPrimitiveIndex();

	ray_gen_record_data* rg_data = (ray_gen_record_data*)optixGetSbtDataPointer();

	custom_gas_data* cgd = (custom_gas_data*)(optixGetGASPointerFromHandle(optixGetGASTraversableHandle()) - sizeof(custom_gas_data));
	uint3 index_triplet = cgd->indices[primitive_idx];
	float3 normal =
		float3_normalize(optixTransformNormalFromObjectToWorldSpace(
			cgd->normals[index_triplet.x] * bary_coords.z +
			cgd->normals[index_triplet.y] * bary_coords.x +
			cgd->normals[index_triplet.z] * bary_coords.y)
		);

	float2 uv = { 0,0 };

	if (cgd->uvs > 0)
	{
		uv = cgd->uvs[index_triplet.x] * bary_coords.z + cgd->uvs[index_triplet.y] * bary_coords.x + cgd->uvs[index_triplet.z] * bary_coords.y;
	}

	size_t pixel_idx = (launch_index.y * lp.render_width * 4) + (launch_index.x * 4);
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		if (lp.passes[p].layer == EXR_LAYER_NORMAL)
		{
			lp.passes[p].pixels[pixel_idx] += normal.x;
			lp.passes[p].pixels[pixel_idx + 1] += normal.y;
			lp.passes[p].pixels[pixel_idx + 2] += normal.z;
			lp.passes[p].pixels[pixel_idx + 3] += 1.f;
		}
		else if (lp.passes[p].layer == EXR_LAYER_UV)
		{
			lp.passes[p].pixels[pixel_idx] += uv.x;
			lp.passes[p].pixels[pixel_idx + 1] += uv.y;
			lp.passes[p].pixels[pixel_idx + 2] += 0;
			lp.passes[p].pixels[pixel_idx + 3] += 1.f;
		}
	}
}

extern "C" __global__ void __miss__ms()
{
	miss_record_data* ms_data = (miss_record_data*)optixGetSbtDataPointer();
}