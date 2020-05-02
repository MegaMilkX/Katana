#version 450
in vec3 Position;
uniform mat4 mat_projection;
uniform mat4 mat_view;
uniform mat4 mat_model;
void main()
{
    gl_Position = mat_projection * mat_view * mat_model * vec4(Position, 1.0);
}