#include "material.h"
#include <string.h>


material material_create(cgltf_material* mat)
{
    material m = {
        .alpha_cutoff = mat->alpha_cutoff,
        .double_sided = mat->double_sided,
    };

    printf("%s\n", mat->name);
    if (mat->has_pbr_metallic_roughness)
    {
        cgltf_pbr_metallic_roughness pbr_mr = mat->pbr_metallic_roughness;
    }

    return m;
}

void material_get_color(material mat, ray r, triangle tri, vec4 out_color)
{
}

void material_destroy(material m)
{
}
