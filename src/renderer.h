#pragma once

#include "scene.h"
#include <stdint.h>

void renderer_render(const float render_width, const float render_height, const uint8_t num_samples, scene scene, uint8_t* pixels);