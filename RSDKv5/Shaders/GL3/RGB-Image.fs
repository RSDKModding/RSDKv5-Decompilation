// =======================
// VARIABLES
// =======================
in_F vec2 ex_UV;
in_F vec4 ex_color;

uniform sampler2D texDiffuse; // screen display texture

uniform vec2 pixelSize;   // internal game resolution (usually 424x240 or smth)
uniform vec2 textureSize; // size of the internal framebuffer texture
uniform vec2 viewSize;    // window viewport size
#if RETRO_REV02  // if RETRO_REV02 is defined it assumes the engine is plus/rev02 RSDKv5, else it assumes pre-plus/Rev01 RSDKv5
uniform float screenDim; // screen dimming percent
#endif


void main()
{
    gl_FragColor = texture2D(texDiffuse, ex_UV);
#if RETRO_REV02 
    gl_FragColor.rgb *= screenDim;
#endif
}