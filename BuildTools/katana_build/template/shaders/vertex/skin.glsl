#version 450
#define MAX_BONE_COUNT 100
in vec3 Position;
in vec2 UV;
in vec3 Normal;
in vec4 BoneIndex4;
in vec4 BoneWeight4;

out vec3 fs_NormalModel;
out vec3 ViewDir;

uniform mat4 mat_projection;
uniform mat4 mat_view;
uniform mat4 mat_model;

uniform mat4 inverseBindPose[MAX_BONE_COUNT];
uniform mat4 bones[MAX_BONE_COUNT];

void main() {
    ViewDir = (inverse(mat_view) * vec4(0, 0, -1, 0)).xyz;

    ivec4 bi = ivec4(
        int(BoneIndex4.x), int(BoneIndex4.y),
        int(BoneIndex4.z), int(BoneIndex4.w)
    );
    vec4 w = BoneWeight4;
    if(w.x + w.y + w.z + w.w > 1.0) {
        w = normalize(w);
    }
    vec4 posModel = (
        (bones[bi.x] * inverseBindPose[bi.x]) * vec4(Position, 1.0) * w.x +
        (bones[bi.y] * inverseBindPose[bi.y]) * vec4(Position, 1.0) * w.y +
        (bones[bi.z] * inverseBindPose[bi.z]) * vec4(Position, 1.0) * w.z +
        (bones[bi.w] * inverseBindPose[bi.w]) * vec4(Position, 1.0) * w.w
    );
    vec3 normalSkinned = (
        (bones[bi.x] * inverseBindPose[bi.x]) * vec4(Normal, 0.0) * w.x +
        (bones[bi.y] * inverseBindPose[bi.y]) * vec4(Normal, 0.0) * w.y +
        (bones[bi.z] * inverseBindPose[bi.z]) * vec4(Normal, 0.0) * w.z +
        (bones[bi.w] * inverseBindPose[bi.w]) * vec4(Normal, 0.0) * w.w
    ).xyz;

    fs_NormalModel = normalize(vec4(normalSkinned, 0.0)).xyz;

    gl_Position = 
        mat_projection *
        mat_view *
        posModel;
}