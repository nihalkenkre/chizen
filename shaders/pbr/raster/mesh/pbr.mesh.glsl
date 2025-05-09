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

layout (set = 1, binding = 0) uniform Model {
    mat4 M;
} model;

layout(set = 1, binding = 1) readonly buffer Positions {
    vec3 positions[];
} positions;

layout(set = 1, binding = 2) readonly buffer Meshlets {
    Meshlet meshlets[];
} meshlets;

layout(set = 1, binding = 3) readonly buffer MeshletsVertices {
    uint meshlets_vertices[];
} meshlets_vertices;

layout(set = 1, binding = 4) readonly buffer MeshletsTriangles {
    uint meshlets_triangles[];
} meshlets_triangles;

layout(location = 0) out VertexOutput {
    vec4 color;
} vertex_output[];

void main() {
    Meshlet meshlet = meshlets.meshlets[gl_WorkGroupID.x];
    SetMeshOutputsEXT(meshlet.vertex_count, meshlet.triangle_count);

    if (gl_LocalInvocationIndex < meshlet.triangle_count)
    {
        uint packed = meshlets_triangles.meshlets_triangles[meshlet.triangle_offset + gl_LocalInvocationIndex];
        uint v_idx_0 = (packed >> 0) & 0xFF;
        uint v_idx_1 = (packed >> 8) & 0xFF;
        uint v_idx_2 = (packed >> 16) & 0xFF;
        gl_PrimitiveTriangleIndicesEXT[gl_LocalInvocationIndex] = uvec3(v_idx_0, v_idx_1, v_idx_2);
    }

    if (gl_LocalInvocationIndex < meshlet.vertex_count)
    {
        uint vertex_index = meshlet.vertex_offset + gl_LocalInvocationIndex;
        vertex_index = meshlets_vertices.meshlets_vertices[vertex_index];

        gl_MeshVerticesEXT[gl_LocalInvocationIndex].gl_Position = camera.VP * vec4(positions.positions[vertex_index], 1);
        vertex_output[gl_LocalInvocationIndex].color = vec4(1, 0, 0, 1);
    }
}