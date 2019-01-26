R"(#version 450
    in vec2 uv_frag;
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
        out_albedo = vec4(
            (
                base_color * texture(tex_albedo, uv_frag)
            ).xyz * u_tint, 
            1.0
        );

        vec3 t_normal = mat_tbn * (texture(tex_normal, uv_frag).xyz * 2.0 - 1.0);

        out_normal = vec4(t_normal * 0.5 + 0.5, 1.0);
        out_metallic = vec4(texture(tex_metallic, uv_frag).xxx, 1.0);
        out_roughness = vec4(texture(tex_roughness, uv_frag).xxx, 1.0);
    }
)"