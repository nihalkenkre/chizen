#include "image.h"
#include <stb/stb_image.h>
#include "utils.h"

image image_create(const cgltf_data* gltf_data, cgltf_image* curr_img)
{
	image img = { 0 };

	int num_channels = 0;
	stbi_info_from_memory(
		(stbi_uc*)((size_t)curr_img->buffer_view->buffer->data + (size_t)curr_img->buffer_view->data + curr_img->buffer_view->offset),
		(int)curr_img->buffer_view->size, (int*)&img.width, (int*)&img.height,
		&num_channels);

	if (num_channels == 3 || num_channels == 4)
	{
		num_channels = 4;
	}

	float* pixels = stbi_loadf_from_memory(
		(stbi_uc*)((size_t)curr_img->buffer_view->buffer->data + (size_t)curr_img->buffer_view->data + curr_img->buffer_view->offset),
		(int)curr_img->buffer_view->size, (int*)&img.width, (int*)&img.height,
		NULL, num_channels);

	struct cudaChannelFormatDesc cfd = { 0 };

	if (num_channels == 1)
	{
		cfd = cudaCreateChannelDesc(32, 0, 0, 0, cudaChannelFormatKindFloat);
	}
	else if (num_channels == 2)
	{
		cfd = cudaCreateChannelDesc(32, 32, 0, 0, cudaChannelFormatKindFloat);
	}
	else if (num_channels == 3 || num_channels == 4)
	{
		cfd = cudaCreateChannelDesc(32, 32, 32, 32, cudaChannelFormatKindFloat);
	}

	CU_CHECK("create cuda pixel_array", cudaMallocArray(&img.d_pixel_array, &cfd, img.width, img.height, cudaArrayDefault));
	CU_CHECK("copy pixels to device", cudaMemcpy2DToArray(img.d_pixel_array, 0, 0, 
		pixels, img.width * num_channels * sizeof(float), img.width * num_channels * sizeof(float), img.height, 
		cudaMemcpyHostToDevice));

	stbi_image_free(pixels);

	return img;
}

void image_destroy(image i)
{
	CU_CHECK("dealloc image d_pixel_array", cudaFreeArray(i.d_pixel_array));
}
