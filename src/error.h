#pragma once

#define CHIZEN_RESULT_CHECK(action, func, result)                                            \
	chi_result = func;                                                                        \
	if (chi_result > CHIZEN_RESULT_SUCCESS)                                                   \
	{                                                                                         \
		printf("APP ERR: %s %d %s: %d\nExiting...\n", action, chi_result, __FILE__, __LINE__); \
		result = chi_result;                                                                   \
		if (chi_result == CHIZEN_RESULT_CUDA_ERROR || chi_result == CHIZEN_RESULT_OPTIX_ERROR) \
		{                                                                                      \
			goto gpu_error;                                                                     \
		}                                                                                      \
		else                                                                                   \
		{                                                                                      \
			goto cpu_error;                                                                     \
		}                                                                                      \
	}

#define CU_CHECK(action, func, result)                                                                          \
	cuda_error = func;                                                                                           \
	if (cuda_error > cudaSuccess)                                                                                \
	{                                                                                                            \
		printf("CUDA ERR: %s %s %s: %d\nExiting...\n", action, cudaGetErrorName(cuda_error), __FILE__, __LINE__); \
		result = CHIZEN_RESULT_CUDA_ERROR;                                                                        \
		goto gpu_error;                                                                                           \
	}

#define OPTIX_CHECK(action, func, result)                                                                           \
	optix_result = func;                                                                                             \
	if (optix_result > OPTIX_SUCCESS)                                                                                \
	{                                                                                                                \
		printf("OPTIX ERR: %s %s %s: %d\nExiting...\n", action, optixGetErrorName(optix_result), __FILE__, __LINE__); \
		result = CHIZEN_RESULT_OPTIX_ERROR;                                                                           \
		goto gpu_error;                                                                                               \
	}

#define EXR_CHECK(action, exr_result, result)                               \
	if (exr_result > EXR_ERR_SUCCESS)                                        \
	{                                                                        \
		printf("EXR ERR: %s %d %s %d\n", action, result, __FILE__, __LINE__); \
		result = CHIZEN_RESULT_EXR_ERROR;                                     \
		goto cpu_error;                                                       \
	}

typedef enum CHIZEN_RESULT
{
	CHIZEN_RESULT_SUCCESS,
	CHIZEN_RESULT_USAGE_ERROR,
	CHIZEN_RESULT_FILE_ERROR,
	CHIZEN_RESULT_CUDA_ERROR,
	CHIZEN_RESULT_OPTIX_ERROR,
	CHIZEN_RESULT_EXR_ERROR,
	CHIZEN_RESULT_FILE_NOT_FOUND,
	CHIZEN_RESULT_PERSP_CAM_NOT_FOUND,
	CHIZEN_RESULT_NO_MEMORY_ALLOCED,
} CHIZEN_RESULT;
