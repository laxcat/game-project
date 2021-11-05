$input a_position, a_normal

#include <bgfx_shader.sh>
// #include "shaderlib.sh"

void main() {
	a_normal;
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
