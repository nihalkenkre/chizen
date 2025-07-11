#pragma once

#include <cgltf/cgltf.h>
#include <cglm/include/cglm/cglm.h>

#include "camera.h"

void utils_get_xform_matrix_for_node(cgltf_node *node, mat4 xform);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	void utils_find_pixel_vecs(camera cam, float fov, float render_width, float render_height, vec3 out_pixel_00_loc, vec3 out_pixel_delta_u, vec3 out_pixel_delta_v);

#ifdef __cplusplus
}
#endif // __cplusplus

