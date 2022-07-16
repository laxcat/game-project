$input v_color0

#include <bgfx_shader.sh>
// #include "shaderlib.sh"
uniform vec4 u_materialBaseColor;

void main() {
	gl_FragColor = v_color0 * u_materialBaseColor;
}
