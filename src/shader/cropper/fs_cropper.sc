$input v_texcoord0, v_norm, v_pos

#include <bgfx_shader.sh>
#include "shared_defines.h"
#include "utils.sh"
#include "engine.h"

/*
Blinn-Phong, with simple approximations for roughness/metalic material settings.

Approximation of blinn-phong exponent based on material-roughness.
http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

Metallic factor simply reduces diffuse color.
*/

void main() {

    gl_FragColor = texture2D(s_texColor, v_texcoord0) * u_materialBaseColor;
}
