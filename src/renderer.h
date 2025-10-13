#pragma once

#include "scene.h"
#include "utils.h"
#include "exr.h"
#include "error.h"

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

typedef struct passes_info
{
	exr_pass passes[32];
	size_t passes_count;
	size_t current_sample_count;
} passes_info;

#ifdef __cplusplus
extern "C" {
#endif
	CHIZEN_RESULT renderer_render_gltf(const uint32_t render_width, const uint32_t render_height, const char* gltf_path, passes_info* pi);
#ifdef __cplusplus
}
#endif