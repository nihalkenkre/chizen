#pragma once

#include "texture.h"
#include "material.h"
#include "exr.h"
#include "light.h"

typedef enum RAY_TYPE
{
	RAY_TYPE_OBJECT_DATA,
	RAY_TYPE_LIGHTING_DATA,
	RAY_TYPE_SHADOW,
	RAY_TYPE_MAX,
} RAY_TYPE;

typedef struct launch_params
{
	texture* textures;
	material* materials;
	exr_pass* passes;
	size_t passes_count;
	light* lights;
	size_t lights_count;
	size_t render_width;
	size_t render_height;
	size_t max_bounces;
	size_t current_time;
	void* states; // curandState*
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

typedef struct ch_record_data
{
	float3* normals;
	float2* uvs;
	void* indices;
	OptixIndicesFormat indices_format;
	int32_t material_index;
} ch_record_data;

typedef struct ch_record
{
	__align__(OPTIX_SBT_RECORD_ALIGNMENT)
		char header[OPTIX_SBT_RECORD_HEADER_SIZE];
	ch_record_data data;
} ch_record;

typedef struct ch_infos
{
	ch_record* ch_records;
	size_t count;
} ch_infos;

typedef struct miss_record_data
{
	float3 DUMMY;
} miss_record_data;

typedef struct ms_record
{
	__align__(OPTIX_SBT_RECORD_ALIGNMENT)
		char header[OPTIX_SBT_RECORD_HEADER_SIZE];
	miss_record_data data;
} ms_record;

typedef struct ray
{
	float3 org;
	float3 dir;
	float3 inv_dir;
	uint3 sign;
} ray;
