R"(#version 450
#define MAX_OMNI_LIGHT 10

in vec2 uv_frag;

out vec4 fragOut;

uniform sampler2D tex_albedo;
uniform sampler2D tex_depth;
uniform sampler2D tex_normal;
uniform sampler2D tex_metallic;
uniform sampler2D tex_roughness;
uniform samplerCube tex_environment;
uniform samplerCube tex_ext1;
uniform sampler2D tex_ext0;

uniform mat4 inverse_view_projection;
uniform vec2 viewport_size;

uniform vec3 view_pos;

uniform int light_omni_count;
uniform vec3 light_omni_pos[MAX_OMNI_LIGHT];
uniform vec3 light_omni_col[MAX_OMNI_LIGHT];
uniform float light_omni_radius[MAX_OMNI_LIGHT];

const float PI = 3.14159265359;

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

vec3 calcLightSource(
    vec3 light_pos,
    vec3 light_color, 
    float light_radius,
    vec3 world_pos, 
    vec3 albedo,
    float roughness,
    float metallic,
    vec3 N, 
    vec3 V, 
    vec3 F0
) {
    vec3 L = normalize(light_pos - world_pos);
    vec3 H = normalize(V + L);
    float distance = length(light_pos - world_pos);
    //float attenuation = 1.0 / (distance * distance);
    float attenuation = pow(saturate(1 - pow(distance / light_radius, 4)), 2) / (distance * distance + 1);
    vec3 radiance = light_color * attenuation;

    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

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
    gl_FragDepth = depth;
    depth = depth * 2.0 - 1.0;

    vec3 albedo = texture(tex_albedo, uv_frag).xyz;
    vec3 normal = texture(tex_normal, uv_frag).xyz;
    vec3 position = vec3((gl_FragCoord.xy / viewport_size) * 2.0 - 1.0, depth);
    vec4 wpos4 = inverse_view_projection * vec4(position.xyz, 1.0);
    position = wpos4.xyz / wpos4.w;
    float metallic = texture(tex_metallic, uv_frag).x;
    float roughness = texture(tex_roughness, uv_frag).x;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 light_dir = normalize(vec3(1, 1, -1));
    vec3 light_pos = vec3(0, 0.5, 0);

    float lightness = dot(normal, light_dir);

    vec3 light_color = vec3(0,1,0.8);

    vec3 N = normal;
    vec3 V = normalize(view_pos - position);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < light_omni_count; ++i) {
        Lo += calcLightSource(
            light_omni_pos[i], light_omni_col[i], light_omni_radius[i], position, albedo, roughness, metallic, N, V, F0
        );
    }
    
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);     
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    vec3 irradiance = texture(tex_environment, N).rgb;
    vec3 diffuse = irradiance * albedo;

    vec3 specular;
    {
        vec3 R = reflect(-V, N);
        const float MAX_R_LOD = 4.0;
        vec3 prefilteredColor = texture(tex_ext1, R, roughness * MAX_R_LOD).rgb;
        vec2 envBRDF = texture(tex_ext0, vec2(max(dot(N, V), 0.0), roughness)).rg;
        specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    }

    vec3 ambient = (kD * diffuse + specular);

    vec3 color = albedo * (ambient + Lo);

    color = color / (color + vec3(1));
    //color = pow(color, vec3(1/2.2));

    fragOut = vec4(color, 1.0);
})"