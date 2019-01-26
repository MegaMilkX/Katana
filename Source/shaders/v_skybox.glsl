R"(#version 450
in vec3 Position;
uniform mat4 mat_projection;
uniform mat4 mat_view;
out vec3 local_pos;

void main() {
    local_pos = Position;

    mat4 rot_view = mat4(mat3(mat_view));
    vec4 clip_pos = mat_projection * rot_view * vec4(local_pos, 1.0);
    gl_Position = clip_pos.xyww;
}
)"