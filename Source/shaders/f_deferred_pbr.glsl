R"(#version 450
    in vec2 uv_frag;
    in vec3 normal_model;
    in vec3 frag_pos_world;
    in vec4 base_color;

    out vec4 out_albedo;
    out vec4 out_position;
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
        out_position = vec4(frag_pos_world, 1.0);
        out_normal = vec4(normal_model, 1.0);
        out_metallic = texture(tex_metallic, uv_frag)
        out_roughness = texture(tex_roughness, uv_frag)
    }
)"