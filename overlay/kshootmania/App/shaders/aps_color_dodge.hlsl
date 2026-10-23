// テクスチャのRGB輝度をアルファ値として使用し、描画色(input.color)で塗りつぶすシェーダー

//
//	Textures
//
Texture2D		g_texture0 : register(t0);
SamplerState	g_sampler0 : register(s0);

namespace s3d
{
	//
	//	VS Output / PS Input
	//
	struct PSInput
	{
		float4 position	: SV_POSITION;
		float4 color	: COLOR0;
		float2 uv		: TEXCOORD0;
	};
}

//
//	Constant Buffer
//
cbuffer PSConstants2D : register(b0)
{
	float4 g_colorAdd;
	float4 g_sdfParam;
	float4 g_sdfOutlineColor;
	float4 g_sdfShadowColor;
	float4 g_internal;
}

// The source of BOTH blend layers is this same APS image, not a solid color.
float4 PS(s3d::PSInput input) : SV_TARGET
{
    float4 base = g_texture0.Sample(g_sampler0, input.uv);
    if (base.a <= 0.0) discard;
    float3 dodge = saturate(base.rgb / max(1.0 - base.rgb, 0.000001));
    float3 dodged = lerp(base.rgb, dodge, input.color.r);
    float3 burned = max(dodged + base.rgb - 1.0, 0.0);
    return float4(lerp(dodged, burned, input.color.g), base.a * input.color.a);
}
