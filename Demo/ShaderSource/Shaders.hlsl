struct VertexInput
{
	float3 pos : POSITION;
    float3 norm : NORMAL;
    float2 uv : TEXTURECOORD;
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
    float3 localSpaceNormal : LS_NORMAL;
    float2 uv : TEXTURECOORD;
    float4x4 worldInvTranspose : WORLDINVTRANSPOSE;
};

struct DirectionalLight
{
    float3 direction;
    float1 padding;
    float3 diffuse;
    float1 padding2;
    float3 ambient;
    float1 padding3;
};

cbuffer PerFrame : register(b0, space0)
{
    float4x4 viewProjection;
    DirectionalLight light;
}

cbuffer PerObject : register(b1, space0)
{
    float4x4 world;
    float4x4 worldInvTranspose;
}

VertexOutput vertex(VertexInput input)
{
    float4x4 wvp = mul(viewProjection, world);
	
	VertexOutput output;
    output.pos = mul(wvp, float4(input.pos, 1.f));
    output.localSpaceNormal = input.norm;
    output.uv = input.uv;
    output.worldInvTranspose = worldInvTranspose;
	return output;
}

SamplerState samp: register(s0, space0);
Texture2D<float4> tex : register(t0, space0);

float4 pixel(VertexOutput input) : SV_TARGET
{
    float3 materialCol = float3(tex.Sample(samp, input.uv.xy).xyz);
    
    float3 ambient = materialCol * light.ambient;

    float3 L = normalize(mul(
                                float3(light.direction.x, -light.direction.y, -light.direction.z), 
                                (float3x3) input.worldInvTranspose));
    float LdotN = max(dot(L, input.localSpaceNormal), 0.f);
    float3 diffuse = light.diffuse * LdotN;

    float3 finalColor;
    finalColor = (ambient + diffuse) * materialCol;
    clamp(finalColor.xyz, 0.f, 1.f);

    return float4(finalColor, 1.0f);
}