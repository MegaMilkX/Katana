#version 450

in vec2 uv_frag;

uniform sampler2D tex_0;

out vec4 out_albedo;

void main()
{
    vec4 c = texture(tex_0, uv_frag);
    out_albedo = vec4(c.r, c.r, c.r, c.r);
}