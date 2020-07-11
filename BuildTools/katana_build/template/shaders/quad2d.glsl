#vertex
#version 450

in vec3 Position;
in vec2 UV;
out vec2 uv_frag;

uniform mat4 mat_proj;
uniform mat4 mat_model;
uniform vec2 quad_size;
uniform vec2 origin;

void main()
{
	uv_frag = UV;
	vec3 pos = Position;
	pos.x *= quad_size.x;
	pos.y *= quad_size.y;
	//pos.x -= quad_size.x * origin.x;
	//pos.y -= quad_size.y * origin.y;
	vec4 pos4 = mat_proj * mat_model * vec4(pos, 1.0);
	//pos4.x += origin.x * 2.0f;
	//pos4.y += origin.y * 2.0f;
	gl_Position = pos4;
}

#fragment
#version 450

in vec2 uv_frag;
out vec4 out_albedo;

uniform sampler2D tex_albedo;
uniform vec4      color;

void main()
{
    out_albedo = texture(tex_albedo, uv_frag) * color;
}
