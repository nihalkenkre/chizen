#pragma once

#include "../cpu/ray.h"
#include "triangle.h"
#include <cgltf/cgltf.h>

typedef struct texture_info
{
    size_t index;
    size_t tex_coord;
} texture_info;

typedef struct normal_texture_info
{
    size_t index;
    size_t tex_coord;
    float scale;
} normal_texture_info;

typedef struct pbr_mr_info
{
    vec4 base_color_factor;
    texture_info base_color_info;
    texture_info metallic_roughness_info;
    float metallic_factor;
    float roughness_factor;
} pbr_mr_info;

typedef struct occlusion_texture_info
{
    size_t index;
    size_t tex_coord;
    float strength;
} occlusion_texture_info;

typedef struct material
{
    pbr_mr_info pbr_info;
    normal_texture_info nrm_info;
    occlusion_texture_info occ_info;
    texture_info emissive_props;
    vec4 emissive_factor;
    size_t alpha_mode;
    float alpha_cutoff;
    bool double_sided;
} material;

material material_create(cgltf_material* mat);

void material_get_color(material mat, ray r, triangle tri, vec4 out_color);
void material_destroy(material m);
