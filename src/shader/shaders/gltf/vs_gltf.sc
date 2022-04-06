$input a_position, a_normal, a_texcoord0
$output v_texcoord0, v_norm, v_pos

#include <bgfx_shader.sh>

uniform mat3 u_normModel;

void main() {
    v_pos = vec3(mul(u_model[0], vec4(a_position, 1.0)));
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_texcoord0 = a_texcoord0;
    v_norm = mul(u_normModel, a_normal);
}
