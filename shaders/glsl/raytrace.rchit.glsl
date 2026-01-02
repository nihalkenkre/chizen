#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

struct Material
{
   vec4 base_color_factor;
   ivec4 base_color_index;
};

layout(set=1, binding=4, std430) readonly buffer Materials{
   Material materials[];
} materials;

layout(set=1, binding=5) uniform sampler2D textures[];

struct VertexInfo
{
   vec3 position;
   vec3 normal;
   vec2 uv;
};

layout(buffer_reference, buffer_reference_align=8) readonly buffer VertexInfos {
   VertexInfo vertex_infos[];
};

layout(buffer_reference, buffer_reference_align=4) readonly buffer Position {
   vec3 positions[];
};

layout(buffer_reference, buffer_reference_align=4) readonly buffer Normal {
   vec3 normals[];
};

layout(buffer_reference, buffer_reference_align=4) readonly buffer UV {
   vec2 uvs[];
};

layout(shaderRecordEXT, std430) buffer CH_SBT {
   VertexInfos vertices;
} ch_sbt;

layout(location = 0) rayPayloadInEXT vec3 hit_value;
hitAttributeEXT vec2 attribs;

void main()
{
   const vec3 bary_coords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

   uvec3 index_triplet;
   ch_sbt.vertices.vertex_infos[0].position;

   hit_value = bary_coords;
}