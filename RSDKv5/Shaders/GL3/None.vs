in_V vec3 in_pos;
// in_V vec4 in_color;
in_V vec2 in_UV;
out vec4 ex_color;
out vec2 ex_UV;

void main()
{
    gl_Position = vec4(in_pos, 1.0);
    // ex_color    = in_color;
    ex_color    = vec4(1.0);
    ex_UV       = in_UV;
}