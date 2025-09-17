#pragma once

#include <stdint.h>
#include "error.h"

typedef enum EXR_LAYER_TYPE
{
	EXR_LAYER_TYPE_FINALCOLOR,
	EXR_LAYER_TYPE_DIFFUSE,
	EXR_LAYER_TYPE_SPECULAR,
	EXR_LAYER_TYPE_BASECOLOR,
	EXT_LAYER_TYPE_TRANSMISSION,
	EXR_LAYER_TYPE_NORMAL,
	EXR_LAYER_TYPE_TANGENT,
	EXR_LAYER_TYPE_BINORMAL,
	EXR_LAYER_TYPE_UV,
	EXR_LAYER_TYPE_METALNESS,
	EXR_LAYER_TYPE_ROUGHNESS,
	EXR_LAYER_TYPE_ZDEPTH,
	EXR_LAYER_TYPE_IRRADIANCE,
} EXR_LAYER_TYPE;

typedef struct EXR_LAYER
{
	EXR_LAYER_TYPE type;
	size_t num_channels;
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

CHIZEN_RESULT write_exr(const char* file_path, const size_t render_width, const size_t render_height, const exr_pass* passes, const size_t passes_count);
