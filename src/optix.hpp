#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    void optix_render(const uint32_t render_width, const uint32_t render_height, uint8_t* pixels);

#ifdef __cplusplus
}
#endif // __cplusplus


