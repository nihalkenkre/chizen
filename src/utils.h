#pragma once

#include <cgltf/cgltf.h>
#include <cglm/include/cglm/cglm.h>

void get_xform_matrix_for_node(const cgltf_node *node, mat4 xform);