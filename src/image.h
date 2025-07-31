#pragma once

#include <cgltf/cgltf.h>
#include <optix.h>
#include <cuda_runtime.h>

typedef struct image
{
	cudaArray_t d_pixel_array;
	size_t width;
	size_t height;
} image;

image image_create(const cgltf_data* gltf_data, cgltf_image* curr_img);
void image_destroy(image i);
