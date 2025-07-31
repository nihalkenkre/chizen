#pragma once

#include "scene.h"
#include "utils.h"

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
	void renderer_render_gltf(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, exr_pass* passes, const size_t passes_pixels_count);
#ifdef __cplusplus
}
#endif