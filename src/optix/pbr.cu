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

#define NUM_LD_BOUNCES 12

typedef struct hit_data
{
	float3 onb[3];
	float3 pos;
	float3 nrm;
	float3 base_color;
	float3 emission_color;
	float2 uv;
	float metalness;
	float roughness;
	float transmission;
	float ior;
} hit_data;

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
	float3 final_color;
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

__device__ static float calculate_fresnel_dielectric(float3 in_dir, float3 n, float eta)
{
	float3 i = -in_dir;

	float cos_theta_i = dot(i, n);
	if (cos_theta_i < 0.f)
	{
		n = -n;
		cos_theta_i = -cos_theta_i;
		eta = 1 / eta;
	}

	float sin_2_theta_i = 1.f - powf(cos_theta_i, 2);
	float sin_2_theta_t = sin_2_theta_i / powf(eta, 2);
	if (sin_2_theta_t >= 1)
	{
		return 1;
	}
	float cos_theta_t = clamp(sqrtf(1.f - sin_2_theta_t), -1.f, 1.f);

	float r_parl = ((eta * cos_theta_i) - cos_theta_t) / ((eta * cos_theta_i) + cos_theta_t);
	float r_perp = (cos_theta_i - (eta * cos_theta_t)) / (cos_theta_i + (eta * cos_theta_t));

	return 0.5f * (powf(r_parl, 2) + powf(r_perp, 2));
}

__device__ static bsdf_sample sample_phong(float3 nrm, float3 in_dir, unsigned int r_idx)
{
	float3 v = in_dir;
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

__device__ static bsdf_sample sample_lambert_reflectance(float3* onb, float3 in_dir, unsigned int r_idx)
{
	float3 v = -in_dir;
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

__device__ static bsdf_sample sample_uniform_transmission(float3* onb, float3 in_dir, float roughness, float ior, unsigned int r_idx)
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

	float3 i = -in_dir;
	float3 n = onb[2];
	float cos_theta_i = dot(i, n);
	float eta = ior;
	if (cos_theta_i < 0.f)
	{
		n = -n;
		cos_theta_i = -cos_theta_i;
		eta = 1.f / ior;
	}
	
	float sin_2_theta_i = 1.f - powf(cos_theta_i, 2);
	float sin_2_theta_t = sin_2_theta_i / powf(eta, 2);
	float cos_theta_t = clamp(sqrtf(1.f - sin_2_theta_t), -1.f, 1.f);

	bsdf_sample bs;

	if (sin_2_theta_t >= 1 )
	{
		bs = {
			.ray_dir = reflect(-i, n),
			.brdf = 1,
			.pdf = 1,
		};
	}
	else
	{
		float3 r = -i / eta + (cos_theta_i / eta - cos_theta_t) * n;
		bs = {
			.ray_dir = r,
			.brdf = 1,
			.pdf = 1,
		};
	}

	return bs;
}

__device__ static hit_data get_hit_data(ch_record_data* ch_data, unsigned int primitive_idx)
{
	hit_data hd = {};

	float2 tmp_bary_coords = optixGetTriangleBarycentrics();
	float3 bary_coords = {
		 tmp_bary_coords.x,
		 tmp_bary_coords.y,
		 1.f - tmp_bary_coords.x - tmp_bary_coords.y,
	};

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

	hd.nrm =
		normalize(optixTransformNormalFromObjectToWorldSpace(
			ch_data->normals[index_triplet.x] * bary_coords.z +
			ch_data->normals[index_triplet.y] * bary_coords.x +
			ch_data->normals[index_triplet.z] * bary_coords.y)
		);

	if (ch_data->uvs > 0)
	{
		hd.uv = ch_data->uvs[index_triplet.x] * bary_coords.z + ch_data->uvs[index_triplet.y] * bary_coords.x + ch_data->uvs[index_triplet.z] * bary_coords.y;
	}

	float3 a = abs(hd.nrm.z) > 0.9f ? float3{ 0,1,0 } : float3{ 0,0,1 };
	float3 hit_tngt = normalize(cross(hd.nrm, a));
	
	hd.onb[0] = cross(hd.nrm, hit_tngt);
	hd.onb[1] = hit_tngt;
	hd.onb[2] = hd.nrm;

	hd.pos = (optixGetWorldRayOrigin() + optixGetWorldRayDirection() * optixGetRayTmax());
	hd.base_color = get_base_color(ch_data->material_index, hd.uv);
	hd.metalness = get_metalness(ch_data->material_index, hd.uv);
	hd.roughness = get_roughness(ch_data->material_index, hd.uv);
	hd.transmission = get_transmission(ch_data->material_index, hd.uv);
	hd.ior = get_ior(ch_data->material_index);
	hd.emission_color = get_emission_color(ch_data->material_index, hd.uv);

	return hd;
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
			lp.passes[p].d_pixels[pixel_idx] += pl.base_color.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += pl.base_color.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += pl.base_color.z;
			lp.passes[p].d_pixels[pixel_idx + 3] = 1;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_UV)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.uv.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += pl.uv.y;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_NORMAL)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.normal.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += pl.normal.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += pl.normal.z;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_METALNESS)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.metalness;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_ROUGHNESS)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.roughness;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_IRRADIANCE)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.irradiance.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += pl.irradiance.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += pl.irradiance.z;
		}
	}
}

__device__ static void write_ld_pixels(const ld_payload& pl, uint3 launch_index)
{
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);

		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_FINALCOLOR)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.final_color.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += pl.final_color.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += pl.final_color.z;
			lp.passes[p].d_pixels[pixel_idx + 3] += 1; 
		}
	}
}

extern "C" __global__ void __raygen__rg()
{
	uint3 launch_index = optixGetLaunchIndex();

	unsigned int r_idx = launch_index.y * lp.render_width + launch_index.x;

	od_payload odpl = {};

	ray_gen_record_data* rg_data = (ray_gen_record_data*)optixGetSbtDataPointer();

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

	ld_payload ldpl =
	{
		.throughput = make_float3(1.f),
		.r_idx = r_idx,
		.is_camera_ray = true,
		.bounces_left = NUM_LD_BOUNCES,
	};

	uint2 p_ldpl = split_pointer(&ldpl);
	optixTrace(lp.handle, ray.org, ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
		RAY_TYPE_LIGHTING_DATA, RAY_TYPE_MAX, RAY_TYPE_LIGHTING_DATA, p_ldpl.x, p_ldpl.y);

	for (int16_t b = 0; b < NUM_LD_BOUNCES; ++b)
	{
		if (ldpl.is_hit)
		{
			optixTrace(lp.handle, ldpl.out_ray.org, ldpl.out_ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
				RAY_TYPE_LIGHTING_DATA, RAY_TYPE_MAX, RAY_TYPE_LIGHTING_DATA, p_ldpl.x, p_ldpl.y);
		}
	}

	write_ld_pixels(ldpl, launch_index);
}

extern "C" __global__ void __closesthit__od()
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

extern "C" __global__ void __closesthit__ld()
{
	uint3 launch_index = optixGetLaunchIndex();
	unsigned int primitive_idx = optixGetPrimitiveIndex();
	ch_record_data* ch_data = (ch_record_data*)optixGetSbtDataPointer();

	hit_data hd = get_hit_data(ch_data, primitive_idx);

	ld_payload* pl = (ld_payload*)merge_pointer(optixGetPayload_0(), optixGetPayload_1());

	if (pl->is_camera_ray)
		pl->is_camera_ray = false;

	if (hd.emission_color > 0.f)
	{
		pl->is_hit = false;
		pl->final_color += pl->throughput * hd.emission_color;
		return;
	}
	
	bsdf_sample bs = {};

	if (hd.metalness == 1)
	{
		bs.ray_dir = reflect(optixGetWorldRayDirection(), hd.nrm);
		bs.brdf = 1;
		bs.pdf = 1;
		pl->is_hit = true;
		pl->throughput *= (bs.brdf * hd.base_color) / bs.pdf;
	}
	else
	{
		float fresnel = calculate_fresnel_dielectric(optixGetWorldRayDirection(), hd.nrm, hd.ior);
		if (curand_uniform(((curandState*)lp.states) + pl->r_idx) > fresnel)
		{
			if (curand_uniform(((curandState*)lp.states) + pl->r_idx) > hd.transmission)
			{
				bs = sample_lambert_reflectance(hd.onb, optixGetWorldRayDirection(), pl->r_idx);
				pl->is_hit = true;
			}
			else
			{
				bs = sample_uniform_transmission(hd.onb, optixGetWorldRayDirection(), hd.roughness, hd.ior, pl->r_idx);
				pl->is_hit = true;
			}
			pl->throughput *= (bs.brdf * hd.base_color) / bs.pdf;
		}
		else
		{
			bs.ray_dir = reflect(optixGetWorldRayDirection(), hd.nrm);
			bs.brdf = 1;
			bs.pdf = 1;
			pl->is_hit = true;
			pl->throughput *= (bs.brdf) / bs.pdf;
		}
	}
		
	pl->out_ray.org = hd.pos;
	pl->out_ray.dir = bs.ray_dir;
}

extern "C" __global__ void __miss__od()
{
	od_payload* pl = (od_payload*)merge_pointer(optixGetPayload_0(), optixGetPayload_1());
	pl->is_hit = false;
}

extern "C" __global__ void __miss__ld()
{
	ld_payload* pl = (ld_payload*)merge_pointer(optixGetPayload_0(), optixGetPayload_1());

	if (pl->is_camera_ray)
	{
		pl->is_camera_ray = false;
	}

	pl->is_hit = false;
}