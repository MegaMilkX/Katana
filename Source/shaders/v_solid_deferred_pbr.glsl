R"(#version 450
    in vec3 Position;
    in vec2 UV;
    in vec3 Normal;

    out vec2 uv_frag;
    out vec3 normal_model;
    out vec3 frag_pos_world;
    out vec4 base_color;

    uniform mat4 mat_model;
    uniform mat4 mat_view;
    uniform mat4 mat_projection;

    void main()
    {
        vec4 pos_screen;
        vec4 pos_model = vec4(Position , 1.0);

        frag_pos_world = vec3(mat_model * pos_model); 
        normal_model = normalize ((mat_model * vec4(Normal, 0.0)).xyz); 
        uv_frag = UV;  
        pos_screen = mat_projection * mat_view * mat_model * pos_model; 
        base_color = vec4(1.0, 1.0, 1.0, 1.0);
        gl_Position = pos_screen ; 
    }
)"