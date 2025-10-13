#pragma once

#include <cstdint>

void generate_random_states(void* states, uint32_t render_width, uint32_t rener_height, cudaStream_t stream);
void avg_and_display_ld_pixels(void* passes, uint32_t passes_count, uint32_t render_width, uint32_t render_height, uint32_t sample_count, cudaStream_t stream);
void convert_float_to_uint_pixels(void* passes, uint32_t passes_count, uint32_t render_width, uint32_t render_height, cudaStream_t stream);