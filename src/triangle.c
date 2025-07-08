#include "triangle.h"

triangle triangle_create(mat4 node_xform, vec3 positions[], vec3 normals[], vec2 uvs[], uint32_t* indices, size_t i)
{
    triangle t = { 0 };

    for (size_t i_curr = i; i_curr < i + 3; ++i_curr)
    {
        size_t index = indices[i_curr];
        glm_mat4_mulv3(node_xform, positions[index], 1.f, t.positions[i_curr - i]);

        mat4 node_xform_tr = { 0 };
        glm_mat4_inv(node_xform, node_xform_tr);
        glm_mat4_transpose(node_xform_tr);
        glm_mat4_mulv3(node_xform_tr, normals[index], 1.f, t.normals[i_curr - i]);

        glm_vec3_copy(uvs[index], t.uvs[i_curr - i]);
    }

    return t;
}
