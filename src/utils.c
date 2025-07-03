#include "utils.h"

void get_xform_matrix_for_node(const cgltf_node* node, mat4 xform)
{
    if (node->has_matrix)
    {
        glm_mat4_make(node->matrix, xform);
    }
    else
    {
        glm_mat4_identity(xform);

        if (node->has_translation)
        {
            glm_translate(xform, node->translation);
        }

        if (node->has_rotation)
        {
            vec3 axis = { 0 };
            glm_quat_axis(node->rotation, axis);
            glm_rotate(xform, glm_quat_angle(node->rotation), axis);
        }

        if (node->has_scale)
        {
            glm_scale(xform, node->scale);
        }
    }
}