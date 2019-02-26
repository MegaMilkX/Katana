R"(#version 450
#define MAX_BONE_COUNT 110
in vec3 Position;
in vec2 UV;
in vec3 Normal;
in vec3 Tangent;
in vec3 Bitangent;
in vec4 BoneIndex4;
in vec4 BoneWeight4;

out vec2 uv_frag;
out vec3 normal_model;
out vec3 frag_pos_screen;
out vec4 base_color;
out vec3 view_pos;
out mat3 mat_tbn;

uniform mat4 mat_model;
uniform mat4 mat_view;
uniform mat4 mat_projection;

uniform mat4 inverseBindPose[MAX_BONE_COUNT];
uniform mat4 bones[MAX_BONE_COUNT];

void main()
{
    view_pos = (inverse(mat_view) * vec4(0,0,0,1)).xyz;

    ivec4 bi = ivec4(
        int(BoneIndex4.x), int(BoneIndex4.y),
        int(BoneIndex4.z), int(BoneIndex4.w)
    );
    vec4 w = BoneWeight4;
    if(w.x + w.y + w.z + w.w > 1.0) {
        w = normalize(w);
    }

    vec3 norm_skinned = (
        (bones[bi.x] * inverseBindPose[bi.x]) * vec4(Normal, 0.0) * w.x +
        (bones[bi.y] * inverseBindPose[bi.y]) * vec4(Normal, 0.0) * w.y +
        (bones[bi.z] * inverseBindPose[bi.z]) * vec4(Normal, 0.0) * w.z +
        (bones[bi.w] * inverseBindPose[bi.w]) * vec4(Normal, 0.0) * w.w
    ).xyz;
    vec3 tan_skinned = (
        (bones[bi.x] * inverseBindPose[bi.x]) * vec4(Tangent, 0.0) * w.x +
        (bones[bi.y] * inverseBindPose[bi.y]) * vec4(Tangent, 0.0) * w.y +
        (bones[bi.z] * inverseBindPose[bi.z]) * vec4(Tangent, 0.0) * w.z +
        (bones[bi.w] * inverseBindPose[bi.w]) * vec4(Tangent, 0.0) * w.w
    ).xyz;
    vec3 bitan_skinned = (
        (bones[bi.x] * inverseBindPose[bi.x]) * vec4(Bitangent, 0.0) * w.x +
        (bones[bi.y] * inverseBindPose[bi.y]) * vec4(Bitangent, 0.0) * w.y +
        (bones[bi.z] * inverseBindPose[bi.z]) * vec4(Bitangent, 0.0) * w.z +
        (bones[bi.w] * inverseBindPose[bi.w]) * vec4(Bitangent, 0.0) * w.w
    ).xyz;
    //normal_model = normalize(vec4(norm_skinned, 0.0)).xyz;
    norm_skinned = normalize(norm_skinned);
    tan_skinned = normalize(tan_skinned);
    bitan_skinned = normalize(bitan_skinned);

    vec3 T = tan_skinned;
    vec3 B = bitan_skinned;
    vec3 N = norm_skinned;
    mat_tbn = mat3(T, B, N);

    vec4 pos_model = (
        (bones[bi.x] * inverseBindPose[bi.x]) * vec4(Position, 1.0) * w.x +
        (bones[bi.y] * inverseBindPose[bi.y]) * vec4(Position, 1.0) * w.y +
        (bones[bi.z] * inverseBindPose[bi.z]) * vec4(Position, 1.0) * w.z +
        (bones[bi.w] * inverseBindPose[bi.w]) * vec4(Position, 1.0) * w.w
    );

    vec4 pos_screen = mat_projection * mat_view * pos_model; 

    frag_pos_screen = vec3(pos_screen);  
    uv_frag = UV;  
    
    base_color = vec4(1.0, 1.0, 1.0, 1.0);
    gl_Position = pos_screen ; 
}
)"