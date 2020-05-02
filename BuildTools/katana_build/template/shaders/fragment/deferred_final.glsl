#version 450


in vec2 uv_frag;

uniform sampler2D tex_albedo;
uniform sampler2D tex_depth;
uniform sampler2D tex_lightness;

out vec4 out_albedo;


void main() {
    float depth = texture(tex_depth, uv_frag).x;
    gl_FragDepth = depth;

    vec3 albedo = texture(tex_albedo, uv_frag).xyz;
    vec3 lightness = texture(tex_lightness, uv_frag).xyz;

    vec3 color = albedo * lightness;
    color = color / (color + vec3(1));
    color = pow(color, vec3(1/2.2));
    out_albedo = vec4(color, 1.0);
}


