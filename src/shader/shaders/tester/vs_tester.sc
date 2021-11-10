$input a_position, a_normal
$output v_intensity

#include <bgfx_shader.sh>
// #include "shaderlib.sh"

void main() {
	v_intensity = dot(mul(u_modelViewProj, vec4(a_normal, 1.0)).xyz, vec3(-0.3, -0.3, 0.0));
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}
