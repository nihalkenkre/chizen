#pragma once

#include <optix.h>
#include <cuda_runtime.h>
#include <stdio.h>
#include "camera.h"
#include "image.h"
#include "texture.h"
#include "material.h"

inline void CU_CHECK(const char* action, const cudaError_t result)
{
	if (result > cudaSuccess)
	{
		printf("CUDA ERR %d: %s\nExiting...\n", result, cudaGetErrorName(result));
		exit(result);
	}
}

inline void OPTIX_CHECK(const char* action, const OptixResult result)
{
	if (result > OPTIX_SUCCESS)
	{
		printf("ERR: %s %s\n", action, optixGetErrorName(result));
		exit(result);
	}
}

typedef enum EXR_LAYER_TYPE
{
	EXR_LAYER_TYPE_BEAUTY,
	EXT_LAYER_TYPE_TRANSMISSION,
	EXR_LAYER_TYPE_NORMAL,
	EXR_LAYER_TYPE_UV,
	EXR_LAYER_TYPE_METALNESS,
	EXR_LAYER_TYPE_ROUGHNESS,
} EXR_LAYER_TYPE;

typedef struct EXR_LAYER
{
	EXR_LAYER_TYPE type;
	char name[64];
} EXR_LAYER;

typedef struct exr_pass
{
	union {
		// CPU pixels
		float* pixels;
		// GPU pixels
		float* d_pixels;
	};
	EXR_LAYER layer;
} exr_pass;

typedef struct custom_gas_data
{
	__align__(OPTIX_ACCEL_BUFFER_BYTE_ALIGNMENT)
		float3* normals;
	float2* uvs;
	void* indices;
	OptixIndicesFormat indices_format;
	int32_t material_index;
} custom_gas_data;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	void utils_get_xform_matrix_for_node(cgltf_node* node, mat4 xform);
	void utils_find_pixel_vecs(camera cam, float fov, float render_width, float render_height, vec3 out_pixel_00_loc, vec3 out_pixel_delta_u, vec3 out_pixel_delta_v);

#ifdef __cplusplus
}
#endif // __cplusplus
