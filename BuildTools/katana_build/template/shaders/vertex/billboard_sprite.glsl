#version 450
in vec3 Position;
in vec2 UV;
out vec2 uv_frag;

uniform mat4 mat_projection;
uniform mat4 mat_view;
uniform mat4 mat_model;

uniform vec2 viewport_size;
uniform vec2 sprite_size;

void main()
{
    vec3 center = vec3(mat_model * vec4(0,0,0,1));
    vec4 w_pos4 = mat_projection * mat_view * vec4(center, 1.0);
    w_pos4 /= w_pos4.w;

    w_pos4.xy += Position.xy * vec2(sprite_size.x / viewport_size.x, sprite_size.y / viewport_size.y);

    uv_frag = UV;
    gl_Position = vec4(w_pos4.xyz, 1.0);
}