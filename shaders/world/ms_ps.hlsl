#include "root_sig.hlsl"

struct MeshOutput
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

[RootSignature(ROOTSIG)]
[outputtopology("triangle")]
    [numthreads(1, 1, 1)] void
    msmain(out indices uint3 triangles[1], out vertices MeshOutput vertices[3])
{
    SetMeshOutputCounts(3, 1);
    triangles[0] = uint3(0, 1, 2);

    vertices[0].pos = float4(-0.5, 0.5, 0.0, 1.0);
    vertices[0].col = float3(1.0, 0.0, 0.0);

    vertices[1].pos = float4(0.5, 0.5, 0.0, 1.0);
    vertices[1].col = float3(0.0, 1.0, 0.0);

    vertices[2].pos = float4(0.0, -0.5, 0.0, 1.0);
    vertices[2].col = float3(0.0, 0.0, 1.0);
}

[RootSignature(ROOTSIG)]
float4 psmain(MeshOutput input) : SV_TARGET
{
    return float4(input.col, 1);
}