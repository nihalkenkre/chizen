#pragma once

#include <optix.h>
#include <cuda_runtime.h>
#include <stdio.h>
#include <cgltf/cgltf.h>
#include <cglm/include/cglm/cglm.h>
#include "camera.h"
#include "error.h"

#define CU_CHECK(result)                                                            \
	if (result > cudaSuccess)                                                        \
	{                                                                                \
		printf("CUDA ERR: %s %d %s\n", cudaGetErrorName(result), __LINE__, __FILE__); \
		goto shutdown;                                                                \
	}

#define OPTIX_CHECK(result)                                                           \
	if (result > OPTIX_SUCCESS)                                                        \
	{                                                                                  \
		printf("OPTIX ERR: %s %d %s\n", optixGetErrorName(result), __LINE__, __FILE__); \
		goto shutdown;                                                                  \
	}

#define RESULT_CHECK(result)                                     \
	if (result > RESULT_CODE_SUCCESS)                             \
	{                                                             \
		printf("APP ERR: %d %s %d\n", result, __FILE__, __LINE__); \
		goto shutdown;                                             \
	}

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	void utils_get_xform_matrix_for_node(cgltf_node *node, mat4 xform);
	void utils_find_pixel_vecs(camera cam, float fov, float render_width, float render_height, vec3 out_pixel_00_loc, vec3 out_pixel_delta_u, vec3 out_pixel_delta_v);

#ifdef __cplusplus
}
#endif // __cplusplus
