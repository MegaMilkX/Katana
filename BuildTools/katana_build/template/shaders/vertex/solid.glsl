#version 450
in vec3 Position;
in vec2 UV;
in vec3 Normal;
uniform mat4 mat_projection;
uniform mat4 mat_view;
uniform mat4 mat_model;
out vec2 UVFrag;
out vec3 NormalFrag;
out vec3 ViewDir;
void main()
{
    ViewDir = (inverse(mat_view) * vec4(0, 0, -1, 0)).xyz;
    UVFrag = UV;
    NormalFrag = normalize((mat_model * vec4(Normal, 0.0)).xyz);
    gl_Position = mat_projection * mat_view * mat_model * vec4(Position, 1.0);
}