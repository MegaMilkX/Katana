R"(#version 450

in vec3 Position;
in vec3 ColorRGBA;

out vec3 f_color;

uniform mat4 mat_projection;
uniform mat4 mat_view;

void main()
{
    f_color = ColorRGBA;
    gl_Position = mat_projection * mat_view * vec4(Position, 1.0);
})"