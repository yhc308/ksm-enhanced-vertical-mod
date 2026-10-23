// Compatible with OpenSiv3D 0.6.16 forward3d (MIT; see LICENSE.txt).
struct VSInput { float4 position:POSITION; float3 normal:NORMAL; float2 uv:TEXCOORD0; };
struct PSInput { float4 position:SV_POSITION; float3 worldPosition:TEXCOORD0; float2 uv:TEXCOORD1; float3 normal:TEXCOORD2; };
cbuffer VSPerObject:register(b2) { row_major float4x4 g_localToWorld; }
cbuffer VSPerMaterial:register(b3) { float4 g_uvTransform; }
cbuffer WallProjective:register(b4) { float4 g_clip[12]; float4 g_world[12]; }
PSInput VS(VSInput input) {
 PSInput result; int i=(int)input.normal.x;
 result.position=g_clip[i]; result.worldPosition=g_world[i].xyz;
 result.uv=input.uv*g_uvTransform.xy+g_uvTransform.zw;
 result.normal=mul(float3(0,input.normal.yz),(float3x3)g_localToWorld);
 return result;
}
