#version 450
layout(location = 0) out float frag_depth;

in vec2 uv_frag;

uniform sampler2D tex_albedo;

void main()
{
    vec4 t = texture(tex_albedo, uv_frag);
    if (t.a < 0.8) {
        discard;
    }

    frag_depth = gl_FragCoord.z;
}
