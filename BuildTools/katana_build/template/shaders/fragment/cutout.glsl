#version 450

in vec2 uv_frag;

uniform sampler2D tex_0;
uniform sampler2D tex_1;

out vec4 out_albedo;

void main()
{
    vec4 a = texture(tex_0, uv_frag);
    vec4 b = texture(tex_1, uv_frag);

    vec4 col = a - b.xxxx;
    out_albedo = col;
}