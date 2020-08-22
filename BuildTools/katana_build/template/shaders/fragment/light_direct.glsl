#version 450

in vec2 uv_frag;

uniform sampler2D tex_albedo;
uniform sampler2D tex_depth;
uniform sampler2D tex_normal;
uniform sampler2D tex_metallic;
uniform sampler2D tex_roughness;

uniform mat4 inverse_view_projection;
uniform vec2 viewport_size;
uniform vec3 view_pos;

uniform vec3 light_dir;
uniform vec3 light_col;

out vec4 out_lightness;


const float PI = 3.14159265359;


float vectorToDepthValue(vec3 Vec)
{
    vec3 AbsVec = abs(Vec);
    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));

    const float f = 1000.0;
    const float n = 0.1;
    float NormZComp = (f+n) / (f-n) - (2*f*n)/(f-n)/LocalZcomp;
    return (NormZComp + 1.0) * 0.5;
}

float saturate(float value) {
    return clamp(value, 0.0, 1.0);
}
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
vec3 calcLightDir(
    vec3 light_dir,
    vec3 light_color,
    vec3 world_pos, 
    vec3 albedo,
    float roughness,
    float metallic,
    vec3 N, 
    vec3 V, 
    vec3 F0
) {
    vec3 L = light_dir;
    vec3 H = normalize(V + L);
    float attenuation = 1.0;
    vec3 radiance = light_color * attenuation;

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(min(1.0, max(dot(H, V), 0.0)), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}


void main()
{
    float depth = texture(tex_depth, uv_frag).x;
    depth = depth * 2.0 - 1.0;

    vec3 albedo = texture(tex_albedo, uv_frag).xyz;
    vec3 normal = texture(tex_normal, uv_frag).xyz;
    vec3 position = vec3((gl_FragCoord.xy / viewport_size) * 2.0 - 1.0, depth);
    vec4 wpos4 = inverse_view_projection * vec4(position.xyz, 1.0);
    position = wpos4.xyz / wpos4.w;
    float metallic = texture(tex_metallic, uv_frag).x;
    float roughness = texture(tex_roughness, uv_frag).x;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 N = normal;
    vec3 V = normalize(view_pos - position);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = calcLightDir(
        light_dir, light_col, position, albedo, roughness, metallic, N, V, F0
    );

    out_lightness = vec4(Lo, 1.0);
}

