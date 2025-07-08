#include "triangle.h"

triangle triangle_create(vec3 positions[], vec3 normals[], vec2 uvs[], uint32_t* indices, size_t i)
{
    triangle t = { 0 };

    for (size_t i_curr = i; i_curr < i + 3; ++i_curr)
    {
        size_t index = indices[i_curr];
        if (positions != NULL) {
            glm_vec3_copy(positions[index], t.positions[i_curr - i]);
        }

        if (normals != NULL) {
            glm_vec3_copy(normals[index], t.normals[i_curr - i]);
        }

        if (uvs != NULL) {
            glm_vec3_copy(uvs[index], t.uvs[i_curr - i]);
        }
    }

    return t;
}
