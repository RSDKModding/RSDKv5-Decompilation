#version 450
// =======================
// VARIABLES
// =======================
layout(location = 0) in vec2 ex_UV;
layout(location = 1) in vec4 ex_color;

layout(location = 0) out vec4 out_frag;

layout(binding = 0) uniform sampler2D texDiffuse; // screen display texture

void main() { out_frag = texture(texDiffuse, ex_UV); }