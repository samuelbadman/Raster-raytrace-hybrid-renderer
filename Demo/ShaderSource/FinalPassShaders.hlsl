struct VertexInput
{
    float3 Pos : POSITION;
    float2 UV : TEXCOORD;
};

struct VertexOutput
{
    float4 Pos : SV_Position;
    float2 UV : TEXCOORD;
};

VertexOutput vertex(VertexInput input)
{
    VertexOutput output;
    output.Pos = float4(input.Pos, 1.f);
    output.UV = input.UV;
    return output;
}

SamplerState samp : register(s0, space0);
Texture2D<float4> gbuffer[2] : register(t0, space0);

float4 pixel(VertexOutput input) : SV_TARGET
{
    float4 sceneTex = gbuffer[0].Sample(samp, float2(input.UV.x, -input.UV.y));
    float4 shadowMap = gbuffer[1].Sample(samp, float2(input.UV.x, -input.UV.y));

    return sceneTex * shadowMap;
}