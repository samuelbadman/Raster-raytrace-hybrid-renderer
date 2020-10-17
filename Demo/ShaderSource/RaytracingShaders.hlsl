RaytracingAccelerationStructure scene : register(t0, space0);
RWTexture2D<float4> output : register(u0, space0);

struct PrimaryRayPayload
{
    float3 col;
};

cbuffer PerFrame : register(b0, space0)
{
    float4x4 inverseView;
    float4x4 inverseProjection;
    float4 lightDirection;
};

[shader("raygeneration")]
void RayGen()
{
    uint2 currentPixel = DispatchRaysIndex().xy;

    uint2 dims = DispatchRaysDimensions().xy;
    float2 d = (((currentPixel + 0.5f) / dims.xy) * 2.f - 1.f);
    
    RayDesc ray;
    ray.Origin = mul(inverseView, float4(0, 0, 0, 1));
    float4 target = mul(inverseProjection, float4(d.x, -d.y, 0, 1));
    ray.Direction = mul(inverseView, float4(target.xyz, 0));
    ray.TMin = 0.0f;
    ray.TMax = 1e+38f;
    
    PrimaryRayPayload payload;
    TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 2, 0, ray, payload);
    
    output[currentPixel.xy] = float4(payload.col, 1.f);
}

[shader("miss")]
void Miss(inout PrimaryRayPayload payload)
{
    payload.col = float3(1, 1, 1);
}

struct ShadowRayPayload
{
    bool InShadow;
};

[shader("closesthit")]
void ClosestHit(inout PrimaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float hitT = RayTCurrent();
    float3 rayDirW = WorldRayDirection();
    float3 rayOriginW = WorldRayOrigin();
    
    float3 hitPosW = rayOriginW + hitT * rayDirW;
    RayDesc shadowRay;
    shadowRay.Origin = hitPosW;
    shadowRay.Direction = normalize(float3(lightDirection.x, -lightDirection.y, -lightDirection.z));
    shadowRay.TMin = 0.01f;
    shadowRay.TMax = 1e+38f;
    
    ShadowRayPayload shadowPayload;
    TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 1, 2, 1, shadowRay, shadowPayload);

    float val = shadowPayload.InShadow ? 0 : 1;
    payload.col = float3(val, val, val);
}

[shader("miss")]
void ShadowMiss(inout ShadowRayPayload payload)
{
    payload.InShadow = false;
}

[shader("closesthit")]
void ShadowClosestHit(inout ShadowRayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.InShadow = true;
}