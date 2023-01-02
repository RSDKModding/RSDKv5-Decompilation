#version 450

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec2 in_UV;

layout (location = 0) out vec2 ex_UV;
layout (location = 1) out vec4 ex_color;

void main()
{
    gl_Position = vec4(in_pos, 1.0);
    ex_color    = in_color;
    ex_UV       = in_UV;
}