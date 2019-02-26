R"(#version 450
in vec2 UVFrag;
in vec3 NormalFrag;
in vec3 ViewDir;
out vec4 out_albedo;
uniform sampler2D tex_albedo;

uniform vec3 u_color;

void main()
{
    vec3 albedo = u_color;
    float lightness = dot(NormalFrag, -ViewDir);
    vec3 n = ((NormalFrag) * 0.5f ) + 0.5f;
    out_albedo = texture(tex_albedo, UVFrag);
})"