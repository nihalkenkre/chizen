#pragma once

#include <optix.h>
#include <cuda_runtime.h>
#include <stdio.h>
#include "camera.h"

inline void CU_CHECK(const char* action, const cudaError_t result)

{
	if (result > cudaSuccess)
	{
		printf("CUDA ERR %d: %s\nExiting...\n", result, action);
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

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	void utils_get_xform_matrix_for_node(cgltf_node* node, mat4 xform);
	void utils_find_pixel_vecs(camera cam, float fov, float render_width, float render_height, vec3 out_pixel_00_loc, vec3 out_pixel_delta_u, vec3 out_pixel_delta_v);

#ifdef __cplusplus
}
#endif // __cplusplus
