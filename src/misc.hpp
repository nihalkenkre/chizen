#pragma once

#include "utils.h"

#ifdef __cplusplus
extern "C" {

#endif // __cplusplus

	void write_exr(const char* file_path, const size_t render_width, const size_t render_height, const exr_pass* passes, const size_t passes_count);

#ifdef __cplusplus

}
#endif // __cplusplus
