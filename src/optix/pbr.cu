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
		unsigned int p0 = 0, p1 = 0, p2 = 0;
		float2 offset = { curand_uniform(((curandState*)rg_data->states) + r_idx), curand_uniform(((curandState*)rg_data->states) + r_idx) };
		ray r = generate_ray(offset, launch_index.x, launch_index.y, rg_data->pixel_00_loc, rg_data->pixel_delta_u, rg_data->pixel_delta_v, rg_data->org);

		optixTrace(lp.handle, r.org, r.dir, 0.1f, 1000.f, 0.f, 0xFF, 0, RAY_TYPE_PRIMARY, 2, 0, p0, p1, p2);

		if (p0 != 0 || p1 != 0 || p2 != 0)
		{
			float3 world_position = float3{ __uint_as_float(p0),
														__uint_as_float(p1),
														__uint_as_float(p2) };

			for (size_t l = 0; l < lp.lights_count; ++l)
			{
				float3 ray_org = world_position;
				float3 ray_dir = normalize(float3{ lp.lights[l].position[0], lp.lights[l].position[1], lp.lights[l].position[2] } - ray_org);
				unsigned int sr_p0 = l;
				optixTrace(lp.handle, ray_org, ray_dir, 0.1f, 1000.f, 0.f, 0xFF, OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT, RAY_TYPE_SHADOW, 2, 1, sr_p0);
			}
		}
	}

	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		for (size_t c = 0; c < lp.passes[p].layer.num_channels; ++c)
		{
			size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);
			lp.passes[p].pixels[pixel_idx + c] /= rg_data->num_samples;
		}
	}
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

	float3 world_position = (optixGetWorldRayOrigin() + optixGetWorldRayDirection() * optixGetRayTmax());
	optixSetPayload_0(__float_as_uint(world_position.x));
	optixSetPayload_1(__float_as_uint(world_position.y));
	optixSetPayload_2(__float_as_uint(world_position.z));

	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);
		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_BASECOLOR)
		{
			if (ch_data->material_index >= 0)
			{
				if (lp.materials[ch_data->material_index].base_tex_idx >= 0)
				{
					float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].base_tex_idx].d_obj, uv.x, uv.y) *
						lp.materials[ch_data->material_index].base_color_factor;

					add_color(p, pixel_idx, color);
				}
				else
				{
					add_color(p, pixel_idx, lp.materials[ch_data->material_index].base_color_factor);
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

					add_color(p, pixel_idx, make_float4(make_float3(color.z), 1.f));
				}
				else
				{
					add_color(p, pixel_idx, make_float4(make_float3(lp.materials[ch_data->material_index].metalness_factor), 1.f));
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

					add_color(p, pixel_idx, make_float4(make_float3(color.y), 1.f));
				}
				else
				{
					add_color(p, pixel_idx, make_float4(make_float3(lp.materials[ch_data->material_index].roughness_factor), 1.f));
				}
			}
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_NORMAL)
		{
			add_color(p, pixel_idx, make_float4(normal, 1.f));
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_UV)
		{
			add_color(p, pixel_idx, make_float4(uv, 0, 1.f));
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_ZDEPTH)
		{
			float z_depth = (length(world_position) - optixGetRayTmin()) / 1000.f;
			add_color(p, pixel_idx, make_float4(
				z_depth,
				z_depth,
				z_depth,
				1.f
			));
		}
	}
}

extern "C" __global__ void __closesthit__sr()
{
	uint3 launch_index = optixGetLaunchIndex();

	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);
		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_IRRADIANCE)
		{
			add_color(p, pixel_idx, float4{ 0,0,0,1 });
		}
	}
}

extern "C" __global__ void __miss__rg()
{
}

extern "C" __global__ void __miss__sr()
{
	uint3 launch_index = optixGetLaunchIndex();
	size_t light_idx = optixGetPayload_0();

	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);
		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_IRRADIANCE)
		{
			float4 color = {
				lp.lights[light_idx].color[0],
				lp.lights[light_idx].color[1],
				lp.lights[light_idx].color[2],
				1.f
			};
			add_color(p, pixel_idx, color);
		}
	}
}
