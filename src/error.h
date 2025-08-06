#pragma once

#define CHIZEN_RESULT_CHECK(chi_result, result)                                     \
	if (chi_result > CHIZEN_RESULT_SUCCESS)                             \
	{                                                             \
		printf("APP ERR: %d %s %d\n", result, __FILE__, __LINE__); \
		result = chi_result;														\
		goto shutdown;                                             \
	}

typedef enum CHIZEN_RESULT
{
	CHIZEN_RESULT_SUCCESS,
	CHIZEN_RESULT_USAGE_ERROR,
	CHIZEN_RESULT_FILE_TYPE_ERROR,
	CHIZEN_RESULT_CUDA_ERROR,
	CHIZEN_RESULT_OPTIX_ERROR,
	CHIZEN_RESULT_EXR_ERROR,
	CHIZEN_RESULT_FILE_NOT_FOUND,
	CHIZEN_RESULT_PERSP_CAM_NOT_FOUND,
} CHIZEN_RESULT;
