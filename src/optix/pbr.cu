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

__device__ static ray generate_ray(float2 offset, const size_t x, const size_t y, float3 pixel_00_loc, float3 pixel_delta_u, float3 pixel_delta_v, float3 org)
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

extern "C" __global__ void __raygen__rg()
{
	uint3 launch_index = optixGetLaunchIndex();
	ray_gen_record_data* rg_data = (ray_gen_record_data*)optixGetSbtDataPointer();

	unsigned int r_idx = threadIdx.x + blockIdx.x * blockDim.x;

	curand_init(launch_index.x + launch_index.y, launch_index.x + launch_index.y, 0, ((curandState*)rg_data->states) + r_idx);

	for (size_t s = 0; s < rg_data->num_samples; ++s)
	{
		float2 offset = { curand_uniform(((curandState*)rg_data->states) + r_idx), curand_uniform(((curandState*)rg_data->states) + r_idx) };
		ray r = generate_ray(offset, launch_index.x, launch_index.y, rg_data->pixel_00_loc, rg_data->pixel_delta_u, rg_data->pixel_delta_v, rg_data->org);

		optixTrace(lp.handle, r.org, r.dir, 0.f, 1000.f, 0.f, 0xFF, 0, 0, 1, 0);
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

__device__ void write_pixels(size_t passes_index, size_t pixel_index, float4 color)
{
	lp.passes[passes_index].d_pixels[pixel_index] += color.x;
	lp.passes[passes_index].d_pixels[pixel_index + 1] += color.y;
	lp.passes[passes_index].d_pixels[pixel_index + 2] += color.z;
	lp.passes[passes_index].d_pixels[pixel_index + 3] += color.w;
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

	closest_hit_record_data* ch_data = (closest_hit_record_data*)optixGetSbtDataPointer();

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

	size_t pixel_idx = (launch_index.y * lp.render_width * 4) + (launch_index.x * 4);
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_BASECOLOR)
		{
			if (ch_data->material_index >= 0)
			{
				if (lp.materials[ch_data->material_index].base_tex_idx >= 0)
				{
					float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].base_tex_idx].d_obj, uv.x, uv.y) *
						lp.materials[ch_data->material_index].base_color_factor;

					write_pixels(p, pixel_idx, color);
				}
				else
				{
					write_pixels(p, pixel_idx, lp.materials[ch_data->material_index].base_color_factor);
				}
			}
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_METALNESS)
		{
			if (ch_data->material_index >= 0)
			{
				if (lp.materials[ch_data->material_index].mr_tex_idx >= 0)
				{
					float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].mr_tex_idx].d_obj, uv.x, uv.y) *
						lp.materials[ch_data->material_index].metalness_factor;

					write_pixels(p, pixel_idx, make_float4(make_float3(color.z), 1.f));
				}
				else
				{
					write_pixels(p, pixel_idx, make_float4(make_float3(lp.materials[ch_data->material_index].metalness_factor), 1.f));
				}
			}
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_ROUGHNESS)
		{
			if (ch_data->material_index >= 0)
			{
				if (lp.materials[ch_data->material_index].mr_tex_idx >= 0)
				{
					float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].mr_tex_idx].d_obj, uv.x, uv.y) *
						lp.materials[ch_data->material_index].roughness_factor;

					write_pixels(p, pixel_idx, make_float4(make_float3(color.y), 1.f));
				}
				else
				{
					write_pixels(p, pixel_idx, make_float4(make_float3(lp.materials[ch_data->material_index].roughness_factor), 1.f));
				}
			}
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_NORMAL)
		{
			write_pixels(p, pixel_idx, make_float4(normal, 1.f));
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_UV)
		{
			write_pixels(p, pixel_idx, make_float4(uv, 0, 1.f));
		}
	}
}

extern "C" __global__ void __miss__ms()
{
	miss_record_data* ms_data = (miss_record_data*)optixGetSbtDataPointer();
}
