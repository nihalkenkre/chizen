
struct Material
{
    float4 base_color_factor;
    int4 base_normal_metalrough_index;
    float4 metalrough_factor;
};

enum LightType
{
    Direction,
    Point,
    Spot,
};

struct Light
{
    float3 position;
    float3 direction;
    float3 color;
    float intensity;
    float range;
    float outerConeAngle;
    float innerConeAngle;
    LightType type;
}