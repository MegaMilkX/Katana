#version 450
in vec2 uv_frag;

out vec4 out_albedo;
uniform sampler2D tex_albedo;

void main()
{
    out_albedo = texture(tex_albedo, uv_frag);
    if (out_albedo.a < 0.8) {
        discard;
    }
}