R"(#version 450
out vec4 out_albedo;

in vec3 local_pos;

uniform samplerCube tex_environment;

void main() {
    vec3 env_color = texture(tex_environment, local_pos).rgb;

    env_color = env_color / (env_color + vec3(1.0));
    env_color = pow(env_color, vec3(1.0/2.2));

    out_albedo = vec4(env_color, 1.0);
}
)"