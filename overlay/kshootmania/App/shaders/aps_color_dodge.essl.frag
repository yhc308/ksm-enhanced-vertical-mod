# version 300 es

precision mediump float;

uniform sampler2D Texture0;

in vec4 Color;
in vec2 UV;

layout(location = 0) out vec4 FragColor;

layout(std140) uniform PSConstants2D
{
	vec4 g_colorAdd;
	vec4 g_sdfParam;
	vec4 g_sdfOutlineColor;
	vec4 g_sdfShadowColor;
	vec4 g_internal;
};

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
