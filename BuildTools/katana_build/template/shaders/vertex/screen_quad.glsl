#version 450
in vec3 Position;
in vec2 UV;
out vec2 uv_frag;
void main()
{
    uv_frag = UV;
    gl_Position = vec4(Position, 1.0);
}