#version 450

out vec4 out_albedo;

in vec3 f_color;

void main()
{
    out_albedo = vec4(f_color, 1.0);
}