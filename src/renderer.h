#pragma once

#include "scene.h"
#include "scene_optix.h"

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

void renderer_render_cpu(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, uint8_t* pixels);

#ifdef __cplusplus
extern "C" {
#endif
	void renderer_render_cuda(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, uint8_t* pixels);
	void renderer_render_optix(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, uint8_t* pixels);
#ifdef __cplusplus
}
#endif