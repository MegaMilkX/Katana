#vertex
#version 450

in vec3 Position;
in vec4 ColorRGBA;

uniform mat4 mat_proj;
uniform mat4 mat_view;
uniform mat4 mat_model;
uniform vec4 color;

out vec4 frag_color;

void main() {
	frag_color = ColorRGBA * color;
    gl_Position = mat_proj * mat_view * mat_model * vec4(Position, 1.0);
}


#fragment
#version 450

in vec4 frag_color;

out vec4 out_albedo;

void main() {
	out_albedo = frag_color;
}