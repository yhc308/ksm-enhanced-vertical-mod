// テクスチャのRGB輝度をアルファ値として使用し、描画色(input.color)で塗りつぶすシェーダー

# version 410

//
//	Textures
//
uniform sampler2D Texture0;

//
//	PSInput
//
layout(location = 0) in vec4 Color;
layout(location = 1) in vec2 UV;

//
//	PSOutput
//
layout(location = 0) out vec4 FragColor;

//
//	Constant Buffer
//
layout(std140) uniform PSConstants2D
{
	vec4 g_colorAdd;
	vec4 g_sdfParam;
	vec4 g_sdfOutlineColor;
	vec4 g_sdfShadowColor;
	vec4 g_internal;
};

//
//	Functions
//
// Both the Color Dodge and Linear Burn sources are the same APS texture.
void main()
{
    vec4 base = texture(Texture0, UV);
    if (base.a <= 0.0) discard;
    vec3 dodge = clamp(base.rgb / max(1.0 - base.rgb, vec3(0.000001)), 0.0, 1.0);
    vec3 dodged = mix(base.rgb, dodge, Color.r);
    vec3 burned = max(dodged + base.rgb - 1.0, vec3(0.0));
    FragColor = vec4(mix(dodged, burned, Color.g), base.a * Color.a);
}
