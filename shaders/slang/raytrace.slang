import "common.slang";

struct CamInfo
{
    float4x4 view_inverse;
    float4x4 proj_inverse;
}

struct PushConstants
{
    uint current_sample;
    uint is_cpu_shading;
};

struct VertexData
{
    float4 tangent;
    float3 normal;
    float2 uv;
};

[[vk::binding(0, 0)]]
ConstantBuffer<CamInfo> cam_info;

[[vk::binding(0, 1)]]
RWTexture2D<float4> accum_target;
[[vk::binding(1, 1)]]
RWTexture2D<float4> final_target;
[[vk::binding(2, 1)]]
RWStructuredBuffer<uint4> rand_states;

[[vk::binding(3, 1)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(4, 1)]]
StructuredBuffer<Material> materials;
[[vk::binding(5, 1)]]
Sampler2D textures[];

[[vk::push_constant]]
PushConstants pc;

struct Payload
{
    float4 hit_value;
};

struct CH_SBT_Record
{
    VertexData *vertex_data;
    uint3 *indices;
    uint64_t material_index;
};

[[vk::shader_record]]
ConstantBuffer<CH_SBT_Record> ch_sbt;

uint TausStep(inout uint z, int S1, int S2, int S3, uint M)
{
    uint b = (((z << S1) ^ z) >> S2);
    return z = (((z & M) << S3) ^ b);
}

uint LCGStep(inout uint z, uint A, uint C)
{
    return z = (A * z + C);
}

float HybridTaus(inout uint4 z)
{
    return 2.3283064365387e-10 * (TausStep(z.x, 13, 19, 12, 4294967294) ^
                                  TausStep(z.y, 2, 25, 4, 4294967288) ^
                                  TausStep(z.z, 3, 11, 17, 4294967280) ^
                                  LCGStep(z.w, 1664525, 1013904223));
}

[shader("raygeneration")]
void raygen()
{
    uint3 launch_id = DispatchRaysIndex();
    uint3 launch_size = DispatchRaysDimensions();

    const float2 pixel_center = float2(launch_id.xy) + float2(0.5, 0.5);
    const float2 in_uv = pixel_center / float2(launch_size.xy);
    float2 d = in_uv * 2.0 - 1.0;
    float4 target = mul(cam_info.proj_inverse, float4(d.x, d.y, 1, 1));

    RayDesc ray_desc;
    ray_desc.Origin = mul(cam_info.view_inverse, float4(0, 0, 0, 1)).xyz;
    ray_desc.Direction = mul(cam_info.view_inverse, float4(normalize(target.xyz), 0)).xyz;
    ray_desc.TMin = 0.01f;
    ray_desc.TMax = 1000.f;

    Payload payload;

    TraceRay(tlas, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 0, 0, ray_desc, payload);

    accum_target[launch_id.xy] += payload.hit_value;
    final_target[launch_id.xy] = accum_target[launch_id.xy] / pc.current_sample;
}

[shader("miss")]
void miss(inout Payload p)
{
    uint3 launch_id = DispatchRaysIndex();
    uint3 launch_size = DispatchRaysDimensions();

    uint flat_index = launch_id.y * launch_size.x + launch_id.x;
    uint4 z = rand_states[flat_index];

    p.hit_value = float4(HybridTaus(z), HybridTaus(z), HybridTaus(z), 1);
    rand_states[flat_index] = z;
}

struct Attributes
{
    float2 bary;
};

[shader("closesthit")]
void closesthit(inout Payload p, in Attributes attribs)
{
    const float3 bary_coords = float3(attribs.bary.x, attribs.bary.y, 1.0f - attribs.bary.x - attribs.bary.y);

    uint3 index_triplet = *(ch_sbt.indices + PrimitiveIndex());

    float2 uv = ch_sbt.vertex_data[index_triplet.x].uv * bary_coords.z +
                ch_sbt.vertex_data[index_triplet.y].uv * bary_coords.x +
                ch_sbt.vertex_data[index_triplet.z].uv * bary_coords.y;

    float4 tangent = ch_sbt.vertex_data[index_triplet.x].tangent * bary_coords.z +
                     ch_sbt.vertex_data[index_triplet.y].tangent * bary_coords.x +
                     ch_sbt.vertex_data[index_triplet.z].tangent * bary_coords.y;

    float3 n = ch_sbt.vertex_data[index_triplet.x].normal * bary_coords.z +
               ch_sbt.vertex_data[index_triplet.y].normal * bary_coords.x +
               ch_sbt.vertex_data[index_triplet.z].normal * bary_coords.y;

    float3 t = (tangent * tangent.w).xyz;
    t = normalize(t - dot(t, n) * n);

    float3 b = cross(n, t);

    t = normalize(mul(ObjectToWorld(), float4(t, 0)).xyz);
    b = normalize(mul(ObjectToWorld(), float4(b, 0)).xyz);
    n = normalize(mul(ObjectToWorld(), float4(n, 0)).xyz);

    float3x3 tbn = transpose(float3x3(t, b, n));

    float4 base_color = materials[ch_sbt.material_index].base_color_factor;

    float3 l = float3(-1, 1, 1);

    if (pc.is_cpu_shading == 0)
    {
        if (materials[ch_sbt.material_index].base_normal_metalrough_index.x >= 0)
        {
            base_color *= textures[NonUniformResourceIndex(materials[ch_sbt.material_index].base_normal_metalrough_index.x)].SampleLevel(uv, 0);
        }

        if (materials[ch_sbt.material_index].base_normal_metalrough_index.y >= 0)
        {
            n = mul(
                tbn,
                textures[NonUniformResourceIndex(materials[ch_sbt.material_index].base_normal_metalrough_index.y)].SampleLevel(uv, 0).xyz * 2 - 1);
        }

        if (materials[ch_sbt.material_index].base_normal_metalrough_index.z >= 0)
        {
            base_color = textures[NonUniformResourceIndex(materials[ch_sbt.material_index].base_normal_metalrough_index.z)].SampleLevel(uv, 0);
        }
    }

    p.hit_value = base_color;
}
