#version 450

in vec2 uv_frag;

uniform sampler2D tex_0;

out vec4 out_albedo;

const float offset = 1.0 / 512.0;


void main()
{
    vec4 col = texture2D(tex_0, uv_frag);
	float a = texture(tex_0, vec2(uv_frag.x + offset, uv_frag.y)).r
		+ texture2D(tex_0, vec2(uv_frag.x, uv_frag.y - offset)).r
		+ texture2D(tex_0, vec2(uv_frag.x - offset, uv_frag.y)).r
		+ texture2D(tex_0, vec2(uv_frag.x, uv_frag.y + offset)).r;
	out_albedo = vec4(a,a,a,a);
}