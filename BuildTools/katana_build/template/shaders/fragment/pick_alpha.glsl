#version 450
in vec2 uv_frag;

out vec4 out_albedo;
uniform sampler2D tex_albedo;

uniform vec4 object_ptr;

void main()
{
    float a = texture(tex_albedo, uv_frag).a;
    out_albedo = vec4(object_ptr.xyz, a);
    if (out_albedo.a < 0.8) {
        discard;
    }
}