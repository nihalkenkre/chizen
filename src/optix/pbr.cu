#include "../common.h"
#include "../primitive.h"
#include <sutil/vec_math.h>

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

typedef struct payload
{
	float4 final_color;
	float4 base_color;
	float3 normal;
	float2 uv;
	float3 irradiance;
	float roughness;
	float metalness;
	float z_depth;
	float3 world_position;
	bool is_hit;
} payload;

typedef union payload_convert
{
	payload* ptr;
	uint2 data;
} payload_convert;

extern "C" __constant__ launch_params lp;

__device__ static uint2 split_pointer(payload* p)
{
	payload_convert pc = {
		.ptr = p,
	};

	return pc.data;
}

__device__ static payload* merge_pointer(unsigned int p0, unsigned int p1)
{
	payload_convert pc = {
		.data = {
			.x = p0,
			.y = p1,
		}
	};

	return pc.ptr;
}

__device__ static ray ray_create(float3 org, float3 dir)
{
	ray r = {
		.org = org,
		.dir = dir,
	};
	r.inv_dir = { 1.f / dir.x, 1.f / dir.y, 1.f / dir.z };
	r.sign = { r.inv_dir.x < 0.f, r.inv_dir.y < 0.f, r.inv_dir.z < 0.f };

	return r;
}

__device__ static ray generate_primary_ray(float2 offset, const size_t x, const size_t y, float3 pixel_00_loc, float3 pixel_delta_u, float3 pixel_delta_v, float3 org)
{
	float3 pixel_delta_u_x = pixel_delta_u * (float)x;
	float3 pixel_delta_v_y = pixel_delta_v * (float)y;

	float3 pixel_delta_u_offset = pixel_delta_u * offset.x;
	float3 pixel_delta_v_offset = pixel_delta_v * offset.y;

	float3 pixel_center = (pixel_00_loc + pixel_delta_u_x);
	pixel_center = pixel_center + pixel_delta_v_y;
	pixel_center = pixel_center + pixel_delta_u_offset;
	pixel_center = pixel_center + pixel_delta_v_offset;

	float3 dir = normalize(pixel_center - org);

	return ray_create(org, dir);
}

__device__ static void write_pixels(const payload& pl, const size_t num_samples)
{
	uint3 launch_index = optixGetLaunchIndex();
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);
		
		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_FINALCOLOR)
		{
			float4 final_color = pl.final_color / num_samples;
			lp.passes[p].d_pixels[pixel_idx] = final_color.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = final_color.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = final_color.z;
			lp.passes[p].d_pixels[pixel_idx + 3] = final_color.w;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_BASECOLOR)
		{
			float4 base_color = pl.base_color / num_samples;
			lp.passes[p].d_pixels[pixel_idx] = base_color.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = base_color.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = base_color.z;
			lp.passes[p].d_pixels[pixel_idx + 3] = base_color.w;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_UV)
		{
			float2 uv = pl.uv / num_samples;
			lp.passes[p].d_pixels[pixel_idx] = uv.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = uv.y;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_NORMAL)
		{
			float3 normal = pl.normal / num_samples;
			lp.passes[p].d_pixels[pixel_idx] = normal.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = normal.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = normal.z;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_METALNESS)
		{
			float metalness = pl.metalness / num_samples;
			lp.passes[p].d_pixels[pixel_idx] = metalness;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_ROUGHNESS)
		{
			float roughness = pl.roughness / num_samples;
			lp.passes[p].d_pixels[pixel_idx] = roughness;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_IRRADIANCE)
		{
			float3 irradiance = pl.irradiance / num_samples;
			lp.passes[p].d_pixels[pixel_idx] = irradiance.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = irradiance.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = irradiance.z;
		}
	}
}

extern "C" __global__ void __raygen__rg()
{
	uint3 launch_index = optixGetLaunchIndex();
	ray_gen_record_data* rg_data = (ray_gen_record_data*)optixGetSbtDataPointer();

	unsigned int r_idx = threadIdx.x + blockIdx.x * blockDim.x;

	curand_init(launch_index.x + launch_index.y, launch_index.x + launch_index.y, 0, ((curandState*)rg_data->states) + r_idx);

	payload pl = {};
	uint2 p = split_pointer(&pl);

	for (size_t s = 0; s < rg_data->num_samples; ++s)
	{
		float2 offset = { curand_uniform(((curandState*)rg_data->states) + r_idx), curand_uniform(((curandState*)rg_data->states) + r_idx) };
		ray r = generate_primary_ray(offset, launch_index.x, launch_index.y, rg_data->pixel_00_loc, rg_data->pixel_delta_u, rg_data->pixel_delta_v, rg_data->org);

		optixTrace(lp.handle, r.org, r.dir, 0.1f, 1000.f, 0.f, 0xFF, 0, RAY_TYPE_PRIMARY, RAY_TYPE_MAX, RAY_TYPE_PRIMARY, p.x, p.y);

		if (pl.is_hit)
		{
			//for (size_t l = 0; l < lp.lights_count; ++l)
			//{
			//	float3 ray_org = pl.world_position;
			//	float3 ray_dir = normalize(float3{ lp.lights[l].position[0], lp.lights[l].position[1], lp.lights[l].position[2] } - ray_org);
			//	unsigned int sr_p0 = l;
			//	optixTrace(lp.handle, ray_org, ray_dir, 0.1f, 1000.f, 0.f, 0xFF, OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT, RAY_TYPE_SHADOW, RAY_TYPE_MAX, RAY_TYPE_SHADOW, p.x, p.y, sr_p0);
			//}

			for (size_t b = 0; b < lp.max_bounces; ++b)
			{
				float3 rand_dir = { curand_uniform(((curandState*)rg_data->states) + r_idx), curand_uniform(((curandState*)rg_data->states) + r_idx), curand_uniform(((curandState*)rg_data->states) + r_idx) };

				if (dot(rand_dir, pl.normal) < 0.f)
				{
					rand_dir = -rand_dir;
				}

				optixTrace(lp.handle, pl.world_position, rand_dir, 0.1f, 1000.f, 0.f, 0xFF, 0, RAY_TYPE_BOUNCE, RAY_TYPE_MAX, RAY_TYPE_BOUNCE, p.x, p.y);
			}

			pl.final_color /= lp.max_bounces;
		}
	}

	write_pixels(pl, rg_data->num_samples);
}

__device__ void add_color(size_t p, size_t pixel_index, float4 color)
{
	float col[4] = { color.x, color.y, color.z, color.w };
	for (size_t c = 0; c < lp.passes[p].layer.num_channels; ++c)
	{
		lp.passes[p].pixels[pixel_index + c] += col[c];
	}
}

extern "C" __global__ void __closesthit__rg()
{
	float2 tmp_bary_coords = optixGetTriangleBarycentrics();
	float3 bary_coords = {
		 tmp_bary_coords.x,
		 tmp_bary_coords.y,
		 1.f - tmp_bary_coords.x - tmp_bary_coords.y,
	};
	uint3 launch_index = optixGetLaunchIndex();

	unsigned int primitive_idx = optixGetPrimitiveIndex();

	ch_rg_record_data* ch_data = (ch_rg_record_data*)optixGetSbtDataPointer();

	uint3 index_triplet = {};
	if (ch_data->indices_format == OPTIX_INDICES_FORMAT_UNSIGNED_BYTE3)
	{
		uchar3 tmp = *((uchar3*)ch_data->indices + primitive_idx);
		index_triplet.x = tmp.x;
		index_triplet.y = tmp.y;
		index_triplet.z = tmp.z;
	}
	if (ch_data->indices_format == OPTIX_INDICES_FORMAT_UNSIGNED_SHORT3)
	{
		ushort3 tmp = *((ushort3*)ch_data->indices + primitive_idx);
		index_triplet.x = tmp.x;
		index_triplet.y = tmp.y;
		index_triplet.z = tmp.z;
	}
	else
	{
		index_triplet = *((uint3*)ch_data->indices + primitive_idx);
	}

	float3 normal =
		normalize(optixTransformNormalFromObjectToWorldSpace(
			ch_data->normals[index_triplet.x] * bary_coords.z +
			ch_data->normals[index_triplet.y] * bary_coords.x +
			ch_data->normals[index_triplet.z] * bary_coords.y));

	float2 uv = { 0, 0 };

	if (ch_data->uvs > 0)
	{
		uv = ch_data->uvs[index_triplet.x] * bary_coords.z + ch_data->uvs[index_triplet.y] * bary_coords.x + ch_data->uvs[index_triplet.z] * bary_coords.y;
	}

	payload* pl = merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	pl->uv += uv;
	pl->normal += normal;
	pl->is_hit = true;
	pl->world_position = (optixGetWorldRayOrigin() + optixGetWorldRayDirection() * optixGetRayTmax());

	if (ch_data->material_index >= 0)
	{
		if (lp.materials[ch_data->material_index].base_tex_idx >= 0)
		{
			float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].base_tex_idx].d_obj, uv.x, uv.y) *
				lp.materials[ch_data->material_index].base_color_factor;

			pl->base_color += color;
		}
		else
		{
			pl->base_color += lp.materials[ch_data->material_index].base_color_factor;
		}

		if (lp.materials[ch_data->material_index].mr_tex_idx >= 0)
		{
			float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].mr_tex_idx].d_obj, uv.x, uv.y) *
				lp.materials[ch_data->material_index].metalness_factor;

			pl->metalness += color.z;
		}
		else
		{
			pl->metalness += lp.materials[ch_data->material_index].metalness_factor;
		}

		if (lp.materials[ch_data->material_index].mr_tex_idx >= 0)
		{
			float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].mr_tex_idx].d_obj, uv.x, uv.y) *
				lp.materials[ch_data->material_index].roughness_factor;

			pl->roughness += color.y;
		}
		else
		{
			pl->roughness += lp.materials[ch_data->material_index].roughness_factor;
		}
	}
}

extern "C" __global__ void __closesthit__b()
{
	float2 tmp_bary_coords = optixGetTriangleBarycentrics();
	float3 bary_coords = {
		 tmp_bary_coords.x,
		 tmp_bary_coords.y,
		 1.f - tmp_bary_coords.x - tmp_bary_coords.y,
	};
	uint3 launch_index = optixGetLaunchIndex();

	unsigned int primitive_idx = optixGetPrimitiveIndex();

	ch_rg_record_data* ch_data = (ch_rg_record_data*)optixGetSbtDataPointer();

	uint3 index_triplet = {};
	if (ch_data->indices_format == OPTIX_INDICES_FORMAT_UNSIGNED_BYTE3)
	{
		uchar3 tmp = *((uchar3*)ch_data->indices + primitive_idx);
		index_triplet.x = tmp.x;
		index_triplet.y = tmp.y;
		index_triplet.z = tmp.z;
	}
	if (ch_data->indices_format == OPTIX_INDICES_FORMAT_UNSIGNED_SHORT3)
	{
		ushort3 tmp = *((ushort3*)ch_data->indices + primitive_idx);
		index_triplet.x = tmp.x;
		index_triplet.y = tmp.y;
		index_triplet.z = tmp.z;
	}
	else
	{
		index_triplet = *((uint3*)ch_data->indices + primitive_idx);
	}

	float2 uv = { 0, 0 };

	if (ch_data->uvs > 0)
	{
		uv = ch_data->uvs[index_triplet.x] * bary_coords.z + ch_data->uvs[index_triplet.y] * bary_coords.x + ch_data->uvs[index_triplet.z] * bary_coords.y;
	}

	payload* pl = merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	pl->is_hit = true;

	if (ch_data->material_index >= 0)
	{
		if (lp.materials[ch_data->material_index].base_tex_idx >= 0)
		{
			float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].base_tex_idx].d_obj, uv.x, uv.y) *
				lp.materials[ch_data->material_index].base_color_factor;

			pl->final_color += pl->base_color + (color / lp.max_bounces);
		}
		else
		{
			pl->final_color += pl->base_color + (lp.materials[ch_data->material_index].base_color_factor / lp.max_bounces);
		}
	}
}

extern "C" __global__ void __closesthit__sr()
{
	payload* pl = merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	pl->is_hit = true;
}

extern "C" __global__ void __miss__rg()
{
	payload* pl = merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	pl->is_hit = false;
}

extern "C" __global__ void __miss__b()
{
	payload* pl = merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	pl->is_hit = false;
}

extern "C" __global__ void __miss__sr()
{
	payload* pl = merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	unsigned int light_idx = optixGetPayload_2();

	pl->irradiance += make_float3(lp.lights[light_idx].color[0], lp.lights[light_idx].color[1], lp.lights[light_idx].color[2]);
	pl->is_hit = false;
}
