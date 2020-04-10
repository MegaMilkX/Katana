R"(#version 450
    in vec2 uv_frag;
    in vec4 vertex_rgba;
    in vec3 normal_model;
    in vec3 frag_pos_screen;
    in vec4 base_color;

    in mat3 mat_tbn;

    out vec4 out_albedo;
    out vec4 out_normal;
    out vec4 out_metallic;
    out vec4 out_roughness;

    uniform sampler2D tex_albedo;
	uniform sampler2D tex_normal;
	uniform sampler2D tex_metallic;
	uniform sampler2D tex_roughness;

    uniform vec3 u_tint;

    void main()
    {
        vec4 t = texture(tex_albedo, uv_frag);
        t = t * base_color;

        out_albedo = vec4(t.xyz * u_tint, t.w) * vertex_rgba;
        if (out_albedo.a < 0.8) {
            discard;
        }

        vec3 t_normal = mat_tbn * (texture(tex_normal, uv_frag).xyz * 2.0 - 1.0);
        if(!gl_FrontFacing) {
            t_normal = -t_normal;
        }

        out_normal = vec4(t_normal * 0.5 + 0.5, 1.0);
        out_metallic = vec4(texture(tex_metallic, uv_frag).xxx, 1.0);
        out_roughness = vec4(texture(tex_roughness, uv_frag).xxx, 1.0);
    }
)"