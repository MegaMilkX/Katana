R"(#version 450

#define MAX_BONE_COUNT 256

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

layout (std140) uniform uCommon3d_t {
    mat4 view;
    mat4 projection;
} uCommon3d;

layout (std140) uniform uBones_t {
    mat4 pose[MAX_BONE_COUNT];
} uBones;

void main()
{
    view_pos = (inverse(uCommon3d.view) * vec4(0,0,0,1)).xyz;

    ivec4 bi = ivec4(
        int(BoneIndex4.x), int(BoneIndex4.y),
        int(BoneIndex4.z), int(BoneIndex4.w)
    );
    vec4 w = BoneWeight4;
    if(w.x + w.y + w.z + w.w > 1.0) {
        w = normalize(w);
    }

    mat4 m0 = uBones.pose[bi.x];
    mat4 m1 = uBones.pose[bi.y];
    mat4 m2 = uBones.pose[bi.z];
    mat4 m3 = uBones.pose[bi.w];

    vec3 norm_skinned = (
        m0 * vec4(Normal, 0.0) * w.x +
        m1 * vec4(Normal, 0.0) * w.y +
        m2 * vec4(Normal, 0.0) * w.z +
        m3 * vec4(Normal, 0.0) * w.w
    ).xyz;
    vec3 tan_skinned = (
        m0 * vec4(Tangent, 0.0) * w.x +
        m1 * vec4(Tangent, 0.0) * w.y +
        m2 * vec4(Tangent, 0.0) * w.z +
        m3 * vec4(Tangent, 0.0) * w.w
    ).xyz;
    vec3 bitan_skinned = (
        m0 * vec4(Bitangent, 0.0) * w.x +
        m1 * vec4(Bitangent, 0.0) * w.y +
        m2 * vec4(Bitangent, 0.0) * w.z +
        m3 * vec4(Bitangent, 0.0) * w.w
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
        m0 * vec4(Position, 1.0) * w.x +
        m1 * vec4(Position, 1.0) * w.y +
        m2 * vec4(Position, 1.0) * w.z +
        m3 * vec4(Position, 1.0) * w.w
    );

    vec4 pos_screen = uCommon3d.projection * uCommon3d.view * pos_model; 

    frag_pos_screen = vec3(pos_screen);  
    uv_frag = UV;  
    
    base_color = vec4(1.0, 1.0, 1.0, 1.0);
    gl_Position = pos_screen ;
}
)"