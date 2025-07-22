#pragma once

typedef enum EXR_LAYER
{
	EXR_LAYER_NORMAL,
	EXR_LAYER_UV,
} EXR_LAYER;

typedef struct exr_pass
{
	EXR_LAYER layer;
	float* pixels;
} exr_pass;

#ifdef __cplusplus
extern "C" {

#endif // __cplusplus

	void write_exr(const char* file_path, const size_t render_width, const size_t render_height, const exr_pass* passes, const size_t passes_count);

#ifdef __cplusplus

}
#endif // __cplusplus
