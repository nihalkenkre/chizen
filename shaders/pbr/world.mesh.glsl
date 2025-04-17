#version 460
#extension GL_EXT_mesh_shader : enable

struct Meshlet {
    uint vertex_offset;
    uint triangle_offset;
    uint vertex_count;
    uint triangle_count;
};

layout(local_size_x = 128) in;
layout(triangles, max_vertices = 64, max_primitives = 128) out;

layout(set = 0, binding = 0) uniform Camera {
    mat4 VP;
} camera;

layout(set = 1, binding = 0) readonly buffer Positions {
    vec3 positions[];
} positions;

layout(set = 1, binding = 1) readonly buffer Meshlets {
    Meshlet meshlets[];
} meshlets;

layout(set = 1, binding = 2) readonly buffer MeshletsVertices {
    uint meshlets_vertices[];
} meshlets_vertices;

layout(set = 1, binding = 3) readonly buffer MeshletsTriangles {
    uint meshlets_triangles[];
} meshlets_triangles;

layout(location = 0) out VertexOutput {
    vec4 color;
} vertex_output[];

void main() {
    SetMeshOutputsEXT(meshlets.meshlets[gl_GlobalInvocationID.x].vertex_count, meshlets.meshlets[gl_GlobalInvocationID.x].triangle_count);
}