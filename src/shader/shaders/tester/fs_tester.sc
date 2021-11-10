$input v_intensity

#include <bgfx_shader.sh>
// #include "shaderlib.sh"

void main() {
	gl_FragColor = mix(
		vec4(1.0, 1.0, 1.0, 1.0),
		vec4(0.5, 0.5, 0.5, 1.0),
		v_intensity
	);
}
