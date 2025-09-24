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

#define NUM_SAMPLES 512
#define NUM_DIFF_BOUNCES 4
#define NUM_SPEC_BOUNCES 4

typedef struct bsdf_sample
{
	float3 ray_dir;
	float brdf;
	float pdf;
} bsdf_sample;

typedef struct od_payload
{
	float3 base_color;
	float3 normal;
	float3 tangent;
	float3 binormal;
	float3 irradiance;
	float3 world_position;
	float2 uv;
	float roughness;
	float metalness;
	float eta;
	bool is_inside_object;
	bool is_hit;
} od_payload;

typedef struct ld_payload
{
	ray out_ray;
	float3 diffuse;
	float3 specular;
	float3 base_color;
	float3 emission_color;
	float3 hit_pos;
	float3 hit_nrm;
	float3 throughput;
	float metalness;
	float roughness;
	unsigned int r_idx;
	bool is_hit;
	bool is_camera_ray;
	uint8_t bounces_left;
} ld_payload;

typedef union payload_convert
{
	void* ptr;
	uint2 data;
} payload_convert;

extern "C" __constant__ launch_params lp;

__device__ static bool operator>(const float3& a, const float b)
{
	return a.x > b || a.y > b || a.z > b;
}

__device__ static bool operator<(const float3& a, const float b)
{
	return a.x < b || a.y < b || a.z < b;
}

__device__ static float3 slerp(float3 a, float3 b, float t)
{
	float angle = acosf(dot(a, b));
	float slerp_a = sin((1 - t) * angle) / sin(angle);
	float slerp_b = sin(t * angle) / sin(angle);

	return a * slerp_a + b * slerp_b;
}

__device__ static uint2 split_pointer(void* p)
{
	payload_convert pc = {
		.ptr = p,
	};

	return pc.data;
}

__device__ static void* merge_pointer(unsigned int p0, unsigned int p1)
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

__device__ static float3 get_base_color(int32_t material_index, float2 uv)
{
	if (material_index >= 0)
	{
		if (lp.materials[material_index].base_tex_idx >= 0)
		{
			float4 color = tex2D<float4>(lp.textures[lp.materials[material_index].base_tex_idx].d_obj, uv.x, uv.y);
			return float3(color.x, color.y, color.z) * float3(lp.materials[material_index].base_color_factor.x, lp.materials[material_index].base_color_factor.y, lp.materials[material_index].base_color_factor.z);
		}
		else
		{
			return float3(lp.materials[material_index].base_color_factor.x, lp.materials[material_index].base_color_factor.y, lp.materials[material_index].base_color_factor.z);
		}
	}
	else
	{
		return make_float3(0.f);
	}
}

__device__ static float get_metalness(int32_t material_index, float2 uv)
{
	if (material_index >= 0)
	{
		if (lp.materials[material_index].mr_tex_idx >= 0)
		{
			return tex2D<float4>(lp.textures[lp.materials[material_index].mr_tex_idx].d_obj, uv.x, uv.y).z *
				lp.materials[material_index].metalness_factor;
		}
		else
		{
			return lp.materials[material_index].metalness_factor;
		}
	}
	else
	{
		return 0.f;
	}
}

__device__ static float get_roughness(int32_t material_index, float2 uv)
{
	if (material_index >= 0)
	{
		if (lp.materials[material_index].mr_tex_idx >= 0)
		{
			return tex2D<float4>(lp.textures[lp.materials[material_index].mr_tex_idx].d_obj, uv.x, uv.y).y *
				lp.materials[material_index].roughness_factor;
		}
		else
		{
			return lp.materials[material_index].roughness_factor;
		}
	}
	else
	{
		return 0.f;
	}
}

__device__ static float3 get_emission_color(int32_t material_index, float2 uv)
{
	if (material_index >= 0)
	{
		if (lp.materials[material_index].emissive_tex_idx >= 0)
		{
			float4 value = tex2D<float4>(lp.textures[lp.materials[material_index].emissive_tex_idx].d_obj, uv.x, uv.y);
			return float3(value.x, value.y, value.z) * lp.materials[material_index].emissive_factor;
		}
		else
		{
			return lp.materials[material_index].emissive_factor;
		}
	}
	else
	{
		return make_float3(0.f);
	}
}

__device__ static float get_transmission(int32_t material_index, float2 uv)
{
	if (material_index >= 0)
	{
		if (lp.materials[material_index].trans_tex_idx >= 0)
		{
			float value = tex2D<float4>(lp.textures[lp.materials[material_index].trans_tex_idx].d_obj, uv.x, uv.y).x;
			return value * lp.materials[material_index].transmission_factor;
		}
		else
		{
			return lp.materials[material_index].transmission_factor;
		}
	}
	else
	{
		return 0;
	}
}

__device__ static float get_ior(int32_t material_index)
{
	if (material_index >= 0)
	{
		return lp.materials[material_index].ior;
	}
	else
	{
		return 1.5f;
	}
}

__device__ static float3 point_on_unit_sphere(unsigned int r_idx)
{
	float3 point;

	do {
		point = float3{
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f
		};
	} while (length(point) > 1.f);

	return normalize(point);
}

__device__ static float3 point_in_unit_sphere(unsigned int r_idx, float3 center)
{
	float3 point;

	do {
		point = float3{
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f
		};
	} while (length(point) > 1.f);

	return (point + center);
}

__device__ static bsdf_sample sample_phong(float3 nrm, float3 v, unsigned int r_idx)
{
	float random_u = curand_uniform(((curandState*)lp.states) + r_idx);
	float random_v = curand_uniform(((curandState*)lp.states) + r_idx);
	float n = 300;

	float alpha = acosf(powf(random_u, 1.f / n));
	float phi = 2.f * M_PIf * random_v;

	float3 r = float3{
		cosf(phi) * sinf(alpha),
		sinf(phi) * sinf(alpha),
		cosf(alpha),
	};

	nrm = reflect(v, nrm);
	float3 a = abs(nrm.z) > 0.9f ? float3{ 0,1,0 } : float3{ 0,0,1 };
	float3 t = normalize(cross(nrm, a));

	float3 onb[3] = {
		cross(nrm, t),
		t,
		nrm,
	};

	r = (r.x * onb[0]) + (r.y * onb[1]) + (r.z * onb[2]);

	bsdf_sample bs = {
		.ray_dir = r,
		.brdf = 1,
		.pdf = ((n + 2.f) / (2.f * M_PIf)) * powf(cos(alpha), n),
	};

	return bs;
}

__device__ static bsdf_sample sample_uniform(float3* onb, float3 v, unsigned int r_idx)
{
	float random_u = curand_uniform(((curandState*)lp.states) + r_idx);
	float random_v = curand_uniform(((curandState*)lp.states) + r_idx);

	float theta = acos(random_u);
	float phi = 2 * M_PIf * random_v;

	float3 r = float3{
		cosf(phi) * sinf(theta),
		sinf(phi) * sinf(theta),
		cosf(theta)
	};

	r = (r.x * onb[0]) + (r.y * onb[1]) + (r.z * onb[2]);

	bsdf_sample bs = {
		.ray_dir = r,
		.brdf = max(dot(r, onb[2]), 0.f) / M_PIf,
		.pdf = 1.f / (2.f * M_PIf),
	};

	return bs;
}

__device__ static bsdf_sample sample_lambert_reflectance(float3* onb, float3 v, unsigned int r_idx)
{
	float random_u = curand_uniform(((curandState*)lp.states) + r_idx);
	float random_v = curand_uniform(((curandState*)lp.states) + r_idx);

	float theta = asinf(sqrtf(random_u));
	float phi = 2 * M_PIf * random_v;

	float3 r = float3{
		cosf(phi) * sinf(theta),
		sinf(phi) * sinf(theta),
		cosf(theta)
	};

	r = (r.x * onb[0]) + (r.y * onb[1]) + (r.z * onb[2]);

	bsdf_sample bs = {
		.ray_dir = r,
		.brdf = max(dot(r, onb[2]), 0.f) / M_PIf,
		.pdf = max(dot(r, onb[2]), 0.f) / M_PIf,
	};

	return bs;
}

__device__ static bsdf_sample sample_uniform_transmission(float3* onb, float3 v, float roughness, float ior, unsigned int r_idx)
{
	// float random_u = curand_uniform(((curandState*)lp.states) + r_idx);
	// float random_v = curand_uniform(((curandState*)lp.states) + r_idx);

	// float theta = acos(random_u);
	// float phi = 2 * M_PIf * random_v;

	// float3 r = float3{
	// 	cosf(phi) * sinf(theta),
	// 	sinf(phi) * sinf(theta),
	// 	cosf(theta)
	// };

	// r = (r.x * onb[0]) + (r.y * onb[1]) + (r.z * onb[2]);

	float3 n = onb[2];
	float cos_theta_i = dot(v, n);
	float eta = ior;
	if (cos_theta_i < 0.f)
	{
		n = -n;
		cos_theta_i = -cos_theta_i;
		eta = 1.f / ior;
	}
	
	float sin_2_theta_i = 1.f - powf(cos_theta_i, 2);
	float sin_2_theta_t = sin_2_theta_i / powf(eta, 2);

	bsdf_sample bs;

	float cos_theta_t = sqrtf(1.f - sin_2_theta_t);
	
	float3 r = -v / eta + (cos_theta_i / eta - cos_theta_t) * n;
	
	bs = {
		.ray_dir = r,
		.brdf = 1,
		.pdf = 1,
	};

	return bs;
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

__device__ static void write_od_pixels(const od_payload& pl)
{
	uint3 launch_index = optixGetLaunchIndex();
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);

		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_BASECOLOR)
		{
			float3 base_color = pl.base_color / NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx] = base_color.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = base_color.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = base_color.z;
			lp.passes[p].d_pixels[pixel_idx + 3] = 1;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_UV)
		{
			float2 uv = pl.uv / NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx] = uv.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = uv.y;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_NORMAL)
		{
			float3 normal = pl.normal / NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx] = normal.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = normal.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = normal.z;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_TANGENT)
		{
			float3 tangent = pl.tangent / NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx] = tangent.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = tangent.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = tangent.z;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_BINORMAL)
		{
			float3 binormal = pl.binormal / NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx] = binormal.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = binormal.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = binormal.z;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_METALNESS)
		{
			float metalness = pl.metalness / NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx] = metalness;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_ROUGHNESS)
		{
			float roughness = pl.roughness / NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx] = roughness;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_IRRADIANCE)
		{
			float3 irradiance = pl.irradiance / NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx] = irradiance.x;
			lp.passes[p].d_pixels[pixel_idx + 1] = irradiance.y;
			lp.passes[p].d_pixels[pixel_idx + 2] = irradiance.z;
		}
	}
}

__device__ static void write_ld_pixels(const ld_payload& diff_pl, const ld_payload& spec_pl, uint3 launch_index)
{
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);

		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_DIFFUSE)
		{
			lp.passes[p].d_pixels[pixel_idx] += diff_pl.diffuse.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += diff_pl.diffuse.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += diff_pl.diffuse.z;
			lp.passes[p].d_pixels[pixel_idx + 3] += 1;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_SPECULAR)
		{
			lp.passes[p].d_pixels[pixel_idx] += spec_pl.specular.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += spec_pl.specular.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += spec_pl.specular.z;
			lp.passes[p].d_pixels[pixel_idx + 3] += 1;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_FINALCOLOR)
		{
			lp.passes[p].d_pixels[pixel_idx] += diff_pl.diffuse.x + spec_pl.specular.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += diff_pl.diffuse.y + spec_pl.specular.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += diff_pl.diffuse.z + spec_pl.specular.z;
			lp.passes[p].d_pixels[pixel_idx + 3] += 1;
		}
	}
}

__device__ static void avg_ld_pixels(uint3 launch_index)
{
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);

		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_DIFFUSE ||
			lp.passes[p].layer.type == EXR_LAYER_TYPE_SPECULAR ||
			lp.passes[p].layer.type == EXR_LAYER_TYPE_FINALCOLOR
			)
		{
			lp.passes[p].d_pixels[pixel_idx] /= NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx + 1] /= NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx + 2] /= NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx + 3] /= NUM_SAMPLES;

			lp.passes[p].d_pixels[pixel_idx] = powf(lp.passes[p].d_pixels[pixel_idx], 0.454545f);
			lp.passes[p].d_pixels[pixel_idx + 1] = powf(lp.passes[p].d_pixels[pixel_idx + 1], 0.454545f);
			lp.passes[p].d_pixels[pixel_idx + 2] = powf(lp.passes[p].d_pixels[pixel_idx + 2], 0.454545f);
		}
	}
}

extern "C" __global__ void __raygen__rg()
{
	uint3 launch_index = optixGetLaunchIndex();

	unsigned int r_idx = (blockDim.x * blockDim.y * threadIdx.z) + (blockDim.x * threadIdx.y) + threadIdx.x;

	curand_init(r_idx + lp.current_time, 0, 0, ((curandState*)lp.states) + r_idx);

	od_payload odpl = {};

	ray_gen_record_data* rg_data = (ray_gen_record_data*)optixGetSbtDataPointer();
	for (uint16_t s = 0; s < NUM_SAMPLES; ++s)
	{
		uint2 p_odpl = split_pointer(&odpl);
		float2 offset = {
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f
		};

		ray ray = generate_primary_ray(
			offset, launch_index.x, launch_index.y,
			rg_data->pixel_00_loc,
			rg_data->pixel_delta_u, rg_data->pixel_delta_v,
			rg_data->org
		);

		// optixTrace(lp.handle, ray.org, ray.dir, 0.01f, 1000.f, 0.f, 0xFF, 0,
			// RAY_TYPE_OBJECT_DATA, RAY_TYPE_MAX, RAY_TYPE_OBJECT_DATA, p_odpl.x, p_odpl.y);

		ld_payload ldpl_diff =
		{
			.throughput = make_float3(1.f),
			.r_idx = r_idx,
			.is_camera_ray = true,
			.bounces_left = NUM_DIFF_BOUNCES,
		};

		uint2 p_ldpl_diff = split_pointer(&ldpl_diff);
		optixTrace(lp.handle, ray.org, ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
			RAY_TYPE_DIFFUSE, RAY_TYPE_MAX, RAY_TYPE_DIFFUSE, p_ldpl_diff.x, p_ldpl_diff.y);

		for (int16_t b = 0; b < NUM_DIFF_BOUNCES; ++b)
		{
			if (ldpl_diff.is_hit)
			{
				optixTrace(lp.handle, ldpl_diff.out_ray.org, ldpl_diff.out_ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
					RAY_TYPE_DIFFUSE, RAY_TYPE_MAX, RAY_TYPE_DIFFUSE, p_ldpl_diff.x, p_ldpl_diff.y);
			}
		}

		ld_payload ldpl_spec =
		{
			.throughput = make_float3(1.f),
			.r_idx = r_idx,
			.is_camera_ray = true,
			.bounces_left = NUM_SPEC_BOUNCES,
		};

		uint2 p_ldpl_spec = split_pointer(&ldpl_spec);
		optixTrace(lp.handle, ray.org, ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
			RAY_TYPE_SPECULAR, RAY_TYPE_MAX, RAY_TYPE_SPECULAR, p_ldpl_spec.x, p_ldpl_spec.y);

		for (int16_t b = 0; b < NUM_SPEC_BOUNCES; ++b)
		{
			if (ldpl_spec.is_hit)
			{
				optixTrace(lp.handle, ldpl_spec.out_ray.org, ldpl_spec.out_ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
					RAY_TYPE_SPECULAR, RAY_TYPE_MAX, RAY_TYPE_SPECULAR, p_ldpl_spec.x, p_ldpl_spec.y);
			}
		}

		write_ld_pixels(ldpl_diff, ldpl_spec, launch_index);
	}

	// write_od_pixels(odpl);
	avg_ld_pixels(launch_index);
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

	ch_record_data* ch_data = (ch_record_data*)optixGetSbtDataPointer();

	uint3 index_triplet = {};
	if (ch_data->indices_format == OPTIX_INDICES_FORMAT_UNSIGNED_BYTE3)
	{
		uchar3 tmp = *((uchar3*)ch_data->indices + primitive_idx);
		index_triplet.x = tmp.x;
		index_triplet.y = tmp.y;
		index_triplet.z = tmp.z;
	}
	else if (ch_data->indices_format == OPTIX_INDICES_FORMAT_UNSIGNED_SHORT3)
	{
		ushort3 tmp = *((ushort3*)ch_data->indices + primitive_idx);
		index_triplet.x = tmp.x;
		index_triplet.y = tmp.y;
		index_triplet.z = tmp.z;
	}
	else if (ch_data->indices_format == OPTIX_INDICES_FORMAT_UNSIGNED_INT3)
	{
		index_triplet = *((uint3*)ch_data->indices + primitive_idx);
	}

	float3 hit_nrm =
		normalize(optixTransformNormalFromObjectToWorldSpace(
			ch_data->normals[index_triplet.x] * bary_coords.z +
			ch_data->normals[index_triplet.y] * bary_coords.x +
			ch_data->normals[index_triplet.z] * bary_coords.y));

	float3 a = abs(hit_nrm.x) > 0.9f ? float3{ 0,1,0 } : float3{ 1,0,0 };
	float3 hit_tngt = normalize(cross(hit_nrm, a));

	float2 uv = { 0, 0 };

	if (ch_data->uvs > 0)
	{
		uv = ch_data->uvs[index_triplet.x] * bary_coords.z + ch_data->uvs[index_triplet.y] * bary_coords.x + ch_data->uvs[index_triplet.z] * bary_coords.y;
	}

	od_payload* pl = (od_payload*)merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	pl->uv += uv;
	pl->normal += hit_nrm;
	pl->tangent += hit_tngt;
	pl->binormal += cross(hit_nrm, hit_tngt);
	pl->is_hit = true;
	pl->world_position = (optixGetWorldRayOrigin() + optixGetWorldRayDirection() * optixGetRayTmax());

	if (ch_data->material_index >= 0)
	{
		if (lp.materials[ch_data->material_index].base_tex_idx >= 0)
		{
			float4 color = tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].base_tex_idx].d_obj, uv.x, uv.y);
			pl->base_color += float3(color.x, color.y, color.z) * float3(lp.materials[ch_data->material_index].base_color_factor.x, lp.materials[ch_data->material_index].base_color_factor.y, lp.materials[ch_data->material_index].base_color_factor.z);
		}
		else
		{
			pl->base_color += float3(lp.materials[ch_data->material_index].base_color_factor.x, lp.materials[ch_data->material_index].base_color_factor.y, lp.materials[ch_data->material_index].base_color_factor.z);
		}

		if (lp.materials[ch_data->material_index].mr_tex_idx >= 0)
		{
			pl->metalness += tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].mr_tex_idx].d_obj, uv.x, uv.y).z * lp.materials[ch_data->material_index].metalness_factor;
		}
		else
		{
			pl->metalness += lp.materials[ch_data->material_index].metalness_factor;
		}

		if (lp.materials[ch_data->material_index].mr_tex_idx >= 0)
		{
			pl->roughness += tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].mr_tex_idx].d_obj, uv.x, uv.y).y * lp.materials[ch_data->material_index].roughness_factor;
		}
		else
		{
			pl->roughness += lp.materials[ch_data->material_index].roughness_factor;
		}
	}
}

extern "C" __global__ void __closesthit__diff()
{
	float2 tmp_bary_coords = optixGetTriangleBarycentrics();
	float3 bary_coords = {
		 tmp_bary_coords.x,
		 tmp_bary_coords.y,
		 1.f - tmp_bary_coords.x - tmp_bary_coords.y,
	};

	uint3 launch_index = optixGetLaunchIndex();
	unsigned int primitive_idx = optixGetPrimitiveIndex();
	ch_record_data* ch_data = (ch_record_data*)optixGetSbtDataPointer();

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

	float3 hit_nrm =
		normalize(optixTransformNormalFromObjectToWorldSpace(
			ch_data->normals[index_triplet.x] * bary_coords.z +
			ch_data->normals[index_triplet.y] * bary_coords.x +
			ch_data->normals[index_triplet.z] * bary_coords.y)
		);

	float2 uv = { 0, 0 };

	if (ch_data->uvs > 0)
	{
		uv = ch_data->uvs[index_triplet.x] * bary_coords.z + ch_data->uvs[index_triplet.y] * bary_coords.x + ch_data->uvs[index_triplet.z] * bary_coords.y;
	}

	float3 emission_color = get_emission_color(ch_data->material_index, uv);
	ld_payload* pl = (ld_payload*)merge_pointer(optixGetPayload_0(), optixGetPayload_1());

	if (pl->is_camera_ray)
		pl->is_camera_ray = false;

	if (emission_color > 0.f)
	{
		pl->is_hit = false;
		pl->diffuse += pl->throughput * emission_color;
		return;
	}
	
	float3 hit_pos = (optixGetWorldRayOrigin() + optixGetWorldRayDirection() * optixGetRayTmax());
	float3 base_color = get_base_color(ch_data->material_index, uv);
	float metalness = get_metalness(ch_data->material_index, uv);
	float roughness = get_roughness(ch_data->material_index, uv);
	float transmission = get_transmission(ch_data->material_index, uv);
	float ior = get_ior(ch_data->material_index);
	
	bsdf_sample bs;
	if (curand_uniform(((curandState*)lp.states) + pl->r_idx) <= transmission)
	{
		float3 a = abs(hit_nrm.z) > 0.9f ? float3{ 0,1,0 } : float3{ 0,0,1 };
		float3 hit_tngt = normalize(cross(hit_nrm, a));
	
		float3 onb[3] = {
			cross(hit_nrm, hit_tngt),
			hit_tngt,
			hit_nrm,
		};

		bs = sample_uniform_transmission(onb, -optixGetWorldRayDirection(), roughness, ior, pl->r_idx);
	}
	else 
	{
		float3 a = abs(hit_nrm.x) > 0.9f ? float3{ 0,1,0 } : float3{ 1,0,0 };
		float3 hit_tngt = normalize(cross(hit_nrm, a));
	
		float3 onb[3] = {
			cross(hit_nrm, hit_tngt),
			hit_tngt,
			hit_nrm,
		};

		bs = sample_lambert_reflectance(onb, -optixGetWorldRayDirection(), pl->r_idx);
	}

	pl->is_hit = true;
	pl->out_ray.org = hit_pos;
	pl->out_ray.dir = bs.ray_dir;
	pl->throughput *= (bs.brdf * (base_color * (1 - metalness))) / bs.pdf;
}

extern "C" __global__ void __closesthit__spec()
{
	float2 tmp_bary_coords = optixGetTriangleBarycentrics();
	float3 bary_coords = {
		 tmp_bary_coords.x,
		 tmp_bary_coords.y,
		 1.f - tmp_bary_coords.x - tmp_bary_coords.y,
	};

	uint3 launch_index = optixGetLaunchIndex();
	unsigned int primitive_idx = optixGetPrimitiveIndex();
	ch_record_data* ch_data = (ch_record_data*)optixGetSbtDataPointer();

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

	float3 hit_nrm =
		normalize(optixTransformNormalFromObjectToWorldSpace(
			ch_data->normals[index_triplet.x] * bary_coords.z +
			ch_data->normals[index_triplet.y] * bary_coords.x +
			ch_data->normals[index_triplet.z] * bary_coords.y));

	float2 uv = { 0, 0 };

	if (ch_data->uvs > 0)
	{
		uv = ch_data->uvs[index_triplet.x] * bary_coords.z + ch_data->uvs[index_triplet.y] * bary_coords.x + ch_data->uvs[index_triplet.z] * bary_coords.y;
	}

	ld_payload* pl = (ld_payload*)merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	float3 hit_pos = (optixGetWorldRayOrigin() + optixGetWorldRayDirection() * optixGetRayTmax());

	float3 base_color = get_base_color(ch_data->material_index, uv);
	float3 emission_color = get_emission_color(ch_data->material_index, uv);

	if (pl->is_camera_ray)
		pl->is_camera_ray = false;

	if (dot(hit_nrm, -optixGetWorldRayDirection()) < 0.f)
	{
		pl->is_hit = false;
		return;
	}

	if (emission_color > 0.f)
	{
		pl->is_hit = false;
		pl->specular += pl->throughput * emission_color;
		return;
	}

	bsdf_sample bs = sample_phong(hit_nrm, optixGetWorldRayDirection(), pl->r_idx);

	pl->is_hit = true;
	pl->out_ray.org = hit_pos;
	pl->out_ray.dir = bs.ray_dir;
	pl->throughput *= bs.brdf / bs.pdf;
}

extern "C" __global__ void __miss__rg()
{
	od_payload* pl = (od_payload*)merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	pl->is_hit = false;
}

extern "C" __global__ void __miss__diff()
{
	ld_payload* pl = (ld_payload*)merge_pointer(optixGetPayload_0(), optixGetPayload_1());

	if (pl->is_camera_ray)
	{
		pl->is_camera_ray = false;
	}

	// pl->diffuse += pl->throughput;

	pl->is_hit = false;
}

extern "C" __global__ void __miss__spec()
{
}
