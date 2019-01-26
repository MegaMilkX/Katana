R"(#version 450
    in vec3 Position;
    in vec2 UV;
    in vec3 Normal;
    in vec3 Tangent;
    in vec3 Bitangent;

    out vec2 uv_frag;
    out vec3 normal_model;
    out vec3 frag_pos_screen;
    out vec4 base_color;
    out vec3 view_pos;
    out mat3 mat_tbn;

    uniform mat4 mat_model;
    uniform mat4 mat_view;
    uniform mat4 mat_projection;

    void main()
    {
        view_pos = (inverse(mat_view) * vec4(0,0,0,1)).xyz;

        normal_model = normalize ((mat_model * vec4(Normal, 0.0)).xyz);

        vec3 T = normalize(vec3(mat_model * vec4(Tangent, 0.0)));
        vec3 B = normalize(vec3(mat_model * vec4(Bitangent, 0.0)));
        vec3 N = normal_model;
        mat_tbn = mat3(T, B, N);

        vec4 pos_model = vec4(Position , 1.0);

        vec4 pos_screen = mat_projection * mat_view * mat_model * pos_model; 

        frag_pos_screen = vec3(pos_screen);  
        uv_frag = UV;  
        
        base_color = vec4(1.0, 1.0, 1.0, 1.0);
        gl_Position = pos_screen ; 
    }
)"