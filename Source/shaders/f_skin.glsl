R"(#version 450
in vec2 UVFrag;
in vec3 fs_NormalModel;
in vec3 ViewDir;
out vec4 out_albedo;
uniform sampler2D tex_albedo;
void main()
{
    vec3 albedo = vec3(0.5, 0.5, 0.5);
    float lightness = dot(fs_NormalModel, -ViewDir);
    out_albedo = vec4(albedo * lightness, 1.0);
})"