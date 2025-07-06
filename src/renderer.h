#pragma once

#include "scene.h"
#include <stdint.h>

void renderer_render_cpu(const float render_width, const float render_height, const uint8_t num_samples, scene scene, uint8_t* pixels);

#ifdef __cplusplus
extern "C" {
#endif
    void renderer_render_cuda(const size_t render_width, const size_t render_height, const uint8_t num_samples, scene scene, uint8_t* pixels);
#ifdef __cplusplus
}
#endif