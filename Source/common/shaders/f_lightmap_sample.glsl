R"(#version 450
in vec2 uv_lightmap_frag;
out vec4 out_albedo;
uniform sampler2D tex_lightmap;

void main()
{
    out_albedo = texture(tex_lightmap, uv_lightmap_frag);
})"