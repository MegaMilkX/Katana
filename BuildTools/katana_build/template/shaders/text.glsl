#vertex
#version 450

in vec3 Position;
in vec2 UV;
in float TextUVLookup;
out vec2 uv_frag;
out vec4 color_frag;

uniform mat4 mat_proj;
uniform mat4 mat_model;
uniform sampler2D tex_1;
uniform int lookupTextureWidth;

void main()
{
	vec2 uv_ = texture(tex_1, vec2((TextUVLookup + 0.5) / float(lookupTextureWidth), 0), 0).xy;
	uv_frag = uv_;
	color_frag = vec4(TextUVLookup / float(lookupTextureWidth), 0, 0, 1);
	vec3 pos3 = Position;
	pos3.x = round(pos3.x);
	pos3.y = round(pos3.y);
	vec4 pos = mat_proj * mat_model * vec4(pos3, 1.0);
	gl_Position = pos;
}

#fragment
#version 450

in vec2 uv_frag;
in vec4 color_frag;
out vec4 out_albedo;

uniform sampler2D tex_albedo;

void main()
{
	float c = texture(tex_albedo, uv_frag).x;
    out_albedo = vec4(1, 1, 1, c);
}
