#pragma once

#include "utils.h"

void write_exr(const char* file_path, const size_t render_width, const size_t render_height, const exr_pass* passes, const size_t passes_count);
