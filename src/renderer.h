#pragma once

#include "scene.h"
#include <stdint.h>

void renderer_render_cpu(const size_t render_width, const size_t render_height, const uint8_t num_samples, scene scene, uint8_t* pixels);

#ifdef __cplusplus
extern "C" {
#endif
    void renderer_render_cuda(const size_t render_width, const size_t render_height, const uint8_t num_samples, scene scene, uint8_t* pixels);
    void renderer_render_optix(const size_t render_width, const size_t render_height, const uint8_t num_samples, scene scene, uint8_t* pixels);
#ifdef __cplusplus
}
#endif