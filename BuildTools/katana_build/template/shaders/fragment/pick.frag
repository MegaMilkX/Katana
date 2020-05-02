#version 450
out vec4 out_albedo;
uniform vec4 object_ptr;
void main()
{
    out_albedo = object_ptr;
}