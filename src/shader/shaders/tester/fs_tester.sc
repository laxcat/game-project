$input v_intensity

#include <bgfx_shader.sh>
// #include "shaderlib.sh"

void main() {
	gl_FragColor = mix(
		vec4(0.9, 0.9, 0.9, 1.0),
		vec4(0.2, 0.2, 0.2, 1.0),
		v_intensity
	);
}
