#include "root_sig.hlsl"

struct camera_properties
{
    float4x4 mvp;
};

ConstantBuffer<camera_properties> cam : register(b0);

struct vertex
{
    float3 pos;
};

struct meshlet
{
    uint vertex_offset;
    uint triangle_offset;
    uint vertex_count;
    uint triangle_count;
};

StructuredBuffer<vertex> vertices : register(t0);
StructuredBuffer<meshlet> meshlets : register(t1);
StructuredBuffer<uint> meshlet_vertices : register(t2);
StructuredBuffer<uint> meshlet_triangles : register(t3);

struct MeshOutputVertex
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

[RootSignature(ROOTSIG)]
[outputtopology("triangle")]
[numthreads(128, 1, 1)]void
msmain(uint gtid : SV_GroupThreadID,
        uint gid : SV_GroupID,
        out indices uint3 mesh_output_triangles[128],
        out vertices MeshOutputVertex mesh_output_vertices[64])
{
    meshlet m = meshlets[gid];
    SetMeshOutputCounts(m.vertex_count, m.triangle_count);
    
    if (gtid < m.triangle_count)
    {
        uint packed = meshlet_triangles[m.triangle_offset + gtid];
        
        uint v_idx_0 = packed & 0xFF;
        uint v_idx_1 = (packed >> 8) & 0xFF;
        uint V_idx_2 = (packed >> 16) & 0xFF;
        mesh_output_triangles[gtid] = uint3(v_idx_0, v_idx_1, V_idx_2);
    }
    
    if (gtid < m.vertex_count)
    {
        uint vertex_index = m.vertex_offset + gtid;
        vertex_index = meshlet_vertices[vertex_index];
        
        mesh_output_vertices[gtid].pos = mul(cam.mvp, float4(vertices[vertex_index].pos, 1.0));
       
        float3 col = float3(float(gid & 1), float(gid & 3) / 4, float(gid & 7) / 8);
        mesh_output_vertices[gtid].col = col;
    }
}

[RootSignature(ROOTSIG)]
float4 psmain(MeshOutputVertex input) : SV_TARGET
{
    return float4(input.col, 1);
}