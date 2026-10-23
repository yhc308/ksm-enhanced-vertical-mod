// VS interface follows OpenSiv3D 0.6.16 forward3d.hlsl (MIT).
// Copyright (c) 2008-2025 Ryo Suzuki, 2016-2025 OpenSiv3D Project.
struct VSInput { float4 position:POSITION; float3 normal:NORMAL; float2 uv:TEXCOORD0; };
struct PSInput { float4 position:SV_POSITION; float3 worldPosition:TEXCOORD0; float2 uv:TEXCOORD1; float3 normal:TEXCOORD2; };
cbuffer VSPerView:register(b1) { row_major float4x4 g_worldToProjected; }
cbuffer VSPerObject:register(b2) { row_major float4x4 g_localToWorld; }
cbuffer VSPerMaterial:register(b3) { float4 g_uvTransform; }
cbuffer TunnelWarp:register(b4) { float4 g_shifts[49]; }
PSInput VS(VSInput input) {
 PSInput result;
 float4 world=mul(input.position,g_localToWorld);
 result.position=mul(world,g_worldToProjected);
 // normal.x carries a per-vertex displacement index, never a UV change.
 result.position.xy+=g_shifts[(int)input.normal.x].xy*result.position.w;
 result.worldPosition=world.xyz;
 result.uv=input.uv*g_uvTransform.xy+g_uvTransform.zw;
 result.normal=mul(float3(0,input.normal.yz),(float3x3)g_localToWorld);
 return result;
}
