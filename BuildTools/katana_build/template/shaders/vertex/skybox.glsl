#version 450
in vec3 Position;
layout (std140) uniform uCommon3d_t {
    mat4 view;
    mat4 projection;
} uCommon3d;
out vec3 local_pos;

void main() {
    local_pos = Position;

    mat4 rot_view = mat4(mat3(uCommon3d.view));
    vec4 clip_pos = uCommon3d.projection * rot_view * vec4(local_pos, 1.0);
    gl_Position = clip_pos.xyww;
}
