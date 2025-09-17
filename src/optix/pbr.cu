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

#define NUM_SAMPLES 1024
#define NUM_DIFF_BOUNCES 8

typedef struct bsdf_sample
{
	float3 ray_dir;
	float bsdf;
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
	float z_depth;
	bool is_hit;
} od_payload;

typedef struct ld_payload
{
	ray out_diff_ray;
	float3 final_color;
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
		return { 0.f, 0.f, 0.f };
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
			return float3(value.x, value.y, value.z) * lp.materials[material_index].emissive_factor * lp.materials[material_index].emissive_strength;
		}
		else
		{
			return lp.materials[material_index].emissive_factor * lp.materials[material_index].emissive_strength;
		}
	}
	else
	{
		return float3(0.f, 0.f, 0.f);
	}
}

__device__ static float3 point_on_unit_sphere(unsigned int r_idx)
{
	float3 point;
	
	do {
		point = float3(
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f
		);
	} while (length(point) > 1.f);

	return normalize(point);
}

__device__ static float3 point_in_unit_sphere(unsigned int r_idx, float3 center)
{
	float3 point;
	
	do {
		point = float3(
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f,
			curand_uniform(((curandState*)lp.states) + r_idx) * 2.f - 1.f
		);
	} while (length(point) > 1.f);

	return (point + center);
}

__device__ static bsdf_sample sample_uniform(float3 onb[3], float3 v, unsigned int r_idx)
{
	float random_u = curand_uniform(((curandState*)lp.states) + r_idx);
	float random_v = curand_uniform(((curandState*)lp.states) + r_idx);
	
	float theta = acosf(1 - random_u);
	float phi = 2 * M_PIf * random_v;
	
	float3 r = float3{
		cosf(phi) * sinf(theta),
		sinf(phi) * sinf(theta),
		cosf(theta)
	};
	
	r = (r.x * onb[0]) + (r.y * onb[1]) + (r.z * onb[2]);

	bsdf_sample bs = {
		.ray_dir = r,
		.bsdf = max(dot(r, onb[2]), 0.f),
		.pdf = 1.f / (2 * M_PIf),
	};

	return bs;
}

__device__ static bsdf_sample sample_lambert(float3 onb[3], float3 v, unsigned int r_idx)
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
		.bsdf = max(dot(r, onb[2]), 0.f) / M_PIf,
		.pdf = max(dot(r, onb[2]), 0.f) / M_PIf,
	};

	return bs;
}

__device__ static float dist_d_ggx(float3 n, float3 h, float a2)
{
	float n_dot_h = dot(n, h);
	float n_dot_h_2 = n_dot_h * n_dot_h;

	float num = a2;
	float denom = (n_dot_h_2 * (a2 - 1.f) + 1.f);

	return (num / (M_PIf * denom * denom));
}

__device__ static float dist_g_cook_torrance(float3 n, float3 v, float3 h, float3 l)
{
	float n_dot_h = dot(n, h);
	float v_dot_h = dot(v, h);
	float n_dot_v = dot(n, v);
	float n_dot_l = dot(n, l);

	float ggx_1 = (2 * n_dot_h * n_dot_v) / v_dot_h;
	float ggx_2 = (2 * n_dot_h * n_dot_l) / v_dot_h;

	return min(1.f, min(ggx_1, ggx_2));
}

__device__ static float dist_g_shlickk_ggx(float dot, float roughness)
{
	float r = roughness + 1;
	float k = (r * r) / 8.f;

	float num = dot;
	float denom = dot * (1.f - k) + k;

	return num / denom;
}

__device__ static float dist_g_smith(float3 n, float3 v, float3 h, float3 l, float roughness)
{
	float n_dot_v = max(dot(n, v), 0.f);
	float n_dot_l = max(dot(n, l), 0.f);

	float ggx1 = dist_g_shlickk_ggx(n_dot_v, roughness);
	float ggx2 = dist_g_shlickk_ggx(n_dot_l, roughness);

	return ggx1 * ggx2;
}

__device__ static float3 dist_f_schlick(float3 h, float3 v, float3 f0)
{
	return f0 + (1.f - f0) * powf(1.f - max(dot(v, h), 0.f), 5.f);
}

__device__ static float3 cook_torrance_brdf(float3 base_color, float3 emission_color, float roughness, float metalness, float3 v, float3 n, float3 hit_pos)
{
	float3 ret_val = { 0, 0, 0 };

	for (size_t lt = 0; lt < lp.lights_count; ++lt)
	{
		float a2 = roughness * roughness;

		float3 l_hp = lp.lights[lt].position - hit_pos;
		float3 l = normalize(l_hp);
		float3 h = normalize(v + l);

		float d = dist_d_ggx(n, h, a2);
		float g = dist_g_smith(n, v, h, l, roughness);
		float3 f = dist_f_schlick(h, v, lerp(float3{ 0.04f, 0.04f, 0.04f }, float3{ base_color.x, base_color.y, base_color.z }, metalness));

		float3 num = d * g * f;
		float denom = (4.f * max(dot(n, v), 0.f) * max(dot(n, l), 0.f)) + 0.0001f;

		float3 specular = num / denom;
		float3 radiance = (lp.lights[lt].color * lp.lights[lt].intensity) / (length(l_hp) * length(l_hp));

		float3 ks = f;
		float3 kd = 1 - ks;
		kd *= (1 - metalness);
		float3 diffuse = kd * (base_color / M_PIf);

		ret_val += (((diffuse + specular) * max(dot(n, l), 0.f)) * radiance);

		// ret_val += make_float4(dist_g_smith(n, v, h, l, roughness));
		// ret_val += make_float4(f, 1.f);
		// ret_val += make_float4(dist_g_cook_torrance(n, v, h, l));
	}

	ret_val += emission_color;

	return ret_val;
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

__device__ static void write_ld_pixels(const ld_payload& pl, uint3 launch_index)
{
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);

		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_FINALCOLOR)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.diffuse.x + pl.specular.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += pl.diffuse.y + pl.specular.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += pl.diffuse.z + pl.specular.z;
			lp.passes[p].d_pixels[pixel_idx + 3] += 1;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_DIFFUSE)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.diffuse.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += pl.diffuse.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += pl.diffuse.z;
			lp.passes[p].d_pixels[pixel_idx + 3] += 1;
		}
		else if (lp.passes[p].layer.type == EXR_LAYER_TYPE_SPECULAR)
		{
			lp.passes[p].d_pixels[pixel_idx] += pl.specular.x;
			lp.passes[p].d_pixels[pixel_idx + 1] += pl.specular.y;
			lp.passes[p].d_pixels[pixel_idx + 2] += pl.specular.z;
			lp.passes[p].d_pixels[pixel_idx + 3] += 1;
		}
	}
}

__device__ static void avg_ld_pixels(uint3 launch_index)
{
	for (size_t p = 0; p < lp.passes_count; ++p)
	{
		size_t pixel_idx = (launch_index.y * lp.render_width * lp.passes[p].layer.num_channels) + (launch_index.x * lp.passes[p].layer.num_channels);

		if (lp.passes[p].layer.type == EXR_LAYER_TYPE_FINALCOLOR || 
			lp.passes[p].layer.type == EXR_LAYER_TYPE_DIFFUSE ||
			lp.passes[p].layer.type == EXR_LAYER_TYPE_SPECULAR)
		{
			lp.passes[p].d_pixels[pixel_idx] /= NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx + 1] /= NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx + 2] /= NUM_SAMPLES;
			lp.passes[p].d_pixels[pixel_idx + 3] /= NUM_SAMPLES;

			lp.passes[p].d_pixels[pixel_idx] = powf(lp.passes[p].d_pixels[pixel_idx], 0.454545f);
			lp.passes[p].d_pixels[pixel_idx + 1] = powf(lp.passes[p].d_pixels[pixel_idx + 1], 0.454545f);
			lp.passes[p].d_pixels[pixel_idx + 2] = powf(lp.passes[p].d_pixels[pixel_idx + 2], 0.454545f);
			lp.passes[p].d_pixels[pixel_idx + 3] = powf(lp.passes[p].d_pixels[pixel_idx + 3], 0.454545f);
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
	for (int16_t s = 0; s < NUM_SAMPLES; ++s)
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
			
		optixTrace(lp.handle, ray.org, ray.dir, 0.01f, 1000.f, 0.f, 0xFF, 0,
			RAY_TYPE_OBJECT_DATA, RAY_TYPE_MAX, RAY_TYPE_OBJECT_DATA, p_odpl.x, p_odpl.y);
				
		ld_payload ldpl_diff = 
		{
			.throughput = make_float3(1.f),
			.r_idx = r_idx,
			.is_camera_ray = true,
			.bounces_left = lp.max_bounces,
		};
 		
		uint2 p_ldpl_diff = split_pointer(&ldpl_diff);
		optixTrace(lp.handle, ray.org, ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
			RAY_TYPE_DIFFUSE, RAY_TYPE_MAX, RAY_TYPE_DIFFUSE, p_ldpl_diff.x, p_ldpl_diff.y);

		for (int16_t b = 0; b < lp.max_bounces; ++b)
		{
			if (ldpl_diff.is_hit)
			{
				optixTrace(lp.handle, ldpl_diff.out_diff_ray.org, ldpl_diff.out_diff_ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
					RAY_TYPE_DIFFUSE, RAY_TYPE_MAX, RAY_TYPE_DIFFUSE, p_ldpl_diff.x, p_ldpl_diff.y);
			}
		}

		// ld_payload ldpl_spec = 
		// {
		// 	.final_color = float3(0.f),
		// 	.curr_attenuation = make_float3(1.f),
		// 	.r_idx = r_idx,
		// 	.is_camera_ray = true,
		// 	.bounces_left = lp.max_bounces,
		// };
 		
		// uint2 p_ldpl_spec = split_pointer(&ldpl_spec);
		// optixTrace(lp.handle, ray.org, ray.dir, 0.001f, 1000.f, 0.f, 0xFF, 0,
		// 	RAY_TYPE_SPECULAR, RAY_TYPE_MAX, RAY_TYPE_SPECULAR, p_ldpl_spec.x, p_ldpl_spec.y);

		write_ld_pixels(ldpl_diff, launch_index);
		// write_ld_pixels(ldpl_spec, launch_index);
	}

	write_od_pixels(odpl);
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

	float3 a = abs(hit_nrm.x > 0.9f) ? float3{0,1,0} : float3{1,0,0};
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
			pl->base_color += float3(color.x, color.y, color.z) *	float3(lp.materials[ch_data->material_index].base_color_factor.x, lp.materials[ch_data->material_index].base_color_factor.y, lp.materials[ch_data->material_index].base_color_factor.z);
		}
		else
		{
			pl->base_color += float3(lp.materials[ch_data->material_index].base_color_factor.x, lp.materials[ch_data->material_index].base_color_factor.y, lp.materials[ch_data->material_index].base_color_factor.z);
		}

		if (lp.materials[ch_data->material_index].mr_tex_idx >= 0)
		{
			pl->metalness += tex2D<float4>(lp.textures[lp.materials[ch_data->material_index].mr_tex_idx].d_obj, uv.x, uv.y).z *lp.materials[ch_data->material_index].metalness_factor;
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

	float3 a = abs(hit_nrm.x > 0.9f) ? float3{0,1,0} : float3{1,0,0};
	float3 hit_tngt = normalize(cross(hit_nrm, a));

	float3 onb[3] = {
		cross(hit_nrm, hit_tngt),
		hit_tngt,
		hit_nrm,
	};

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

	pl->is_hit = true;
	pl->diffuse += pl->throughput * emission_color;

	float3 pt_in_sphere = point_in_unit_sphere(pl->r_idx, hit_pos + hit_nrm) - hit_pos;
	pl->out_diff_ray.org = hit_pos;

	bsdf_sample bs = sample_lambert(onb, -optixGetWorldRayDirection(), pl->r_idx);

	pl->out_diff_ray.dir = bs.ray_dir;
	pl->throughput *= (bs.bsdf * base_color) / bs.pdf;
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

	if (--pl->bounces_left > 0)
	{
		if (pl->is_camera_ray)
		{
			pl->is_camera_ray = false;
		}
		else
		{
			pl->throughput = base_color;
		}

		if (emission_color.x > 0 || emission_color.y > 0 || emission_color.z > 0)
		{
			pl->specular += emission_color;
		}
		else
		{
			uint2 p = split_pointer(pl);
			float3 spec_dir = reflect(optixGetWorldRayDirection(), hit_nrm) + (point_on_unit_sphere(pl->r_idx) * get_roughness(ch_data->material_index, uv));
			float3 ray_dir = spec_dir;

			optixTrace(lp.handle, hit_pos, ray_dir, 0.001f, 1000.f, 0.f, 0xFF, 0, RAY_TYPE_SPECULAR, RAY_TYPE_MAX, RAY_TYPE_SPECULAR, p.x, p.y);
			pl->specular *= base_color;
		}
	}
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
		// pl->diffuse = make_float3(0.f);
	}
	else
	{
		// pl->diffuse += pl->throughput;
	}

	pl->is_hit = false;
}

extern "C" __global__ void __miss__spec()
{
}
