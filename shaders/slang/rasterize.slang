struct VSInput
{
    [[vk::location(0)]]
    float3 pos;
    [[vk::location(1)]]
    float3 nrm;
    [[vk::location(2)]]
    float2 uv;
}

struct VSOutput
{
    float4 pos : SV_Position;
    [[vk::location(0)]]
    float3 nrm;
    [[vk::location(1)]]
    float2 uv;
}

struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
}

ConstantBuffer<UniformBuffer> uniform_buffer;

[shader("vertex")]
VSOutput vertex_main(VSInput in)
{
    VSOutput out;
    out.pos = mul(uniform_buffer.proj * uniform_buffer.view * uniform_buffer.model, float4(in.pos, 1));
    out.nrm = in.nrm;
    out.uv = in.uv;

    return out;
}

[[vk::binding(0, 0)]]
Sampler2D diffuse;

[shader("fragment")]
float4 frag_main(VSOutput in)
{
    return diffuse.Sample(in.uv);
}
