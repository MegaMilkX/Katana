#vertex
#version 450

in vec4 Position;
in vec4 ColorRGBA;

out vec3 var_ColorRGBA;

uniform mat4 mat_view;
uniform mat4 mat_proj;

void main() {
	var_ColorRGBA = ColorRGBA.xyz;
	gl_Position = mat_proj * mat_view * vec4(Position.xyz, 1.0);
}

#fragment
#version 450

in vec3 var_ColorRGBA;

out vec4 out_albedo;

void main() {
	out_albedo = vec4(var_ColorRGBA,1);
}