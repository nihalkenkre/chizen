#pragma once

#include <optix.h>
#include <cuda_runtime.h>
#include <stdio.h>

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
