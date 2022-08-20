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
    vec3 yuv;
    yuv.r  = texture2D(texDiffuse, ex_UV).r;
    yuv.gb = texture2D(texDiffuse, clamp(ex_UV / vec2(2.0), 0.0, 0.499)).gb;
    yuv -= vec3(16.0 / 256.0, .5, .5);

    gl_FragColor.r = 1.164 * yuv.r + 1.596 * yuv.b;
    gl_FragColor.g = 1.164 * yuv.r - 0.392 * yuv.g - 0.813 * yuv.b;
    gl_FragColor.b = 1.164 * yuv.r + 2.017 * yuv.g;
#if RETRO_REV02 
    gl_FragColor.rgb *= screenDim;
#endif
}