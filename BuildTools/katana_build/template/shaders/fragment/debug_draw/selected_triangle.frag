#version 450

uniform vec4 u_color;

out vec4 out_color;

void main() {
	float alpha = 1.0;
	if(gl_FragDepth > gl_FragCoord.z) {
		alpha = 0.5;
	}

    out_color = vec4(u_color.xyz, alpha);
}

