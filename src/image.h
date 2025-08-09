#pragma once

#include <cgltf/cgltf.h>
#include <optix.h>
#include <cuda_runtime.h>
#include "error.h"

typedef struct image
{
	cudaArray_t d_pixel_array;
	size_t width;
	size_t height;
	CHIZEN_RESULT result;
} image;

image image_create(const cgltf_data* gltf_data, cgltf_image* curr_img);
CHIZEN_RESULT image_destroy(image i);
