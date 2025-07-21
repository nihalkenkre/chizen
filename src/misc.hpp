#pragma once


#ifdef __cplusplus
extern "C" {

#endif // __cplusplus

	void write_exr(const char* file_path, const size_t render_width, const size_t render_height, float* pixels);

#ifdef __cplusplus

}
#endif // __cplusplus
