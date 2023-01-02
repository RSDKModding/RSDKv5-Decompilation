#version 450
// =======================
// VARIABLES
// =======================
layout (location = 0) in vec2 ex_UV;
layout (location = 1) in vec4 ex_color;

layout (location = 0) out vec4 out_frag;

layout (binding = 0) uniform sampler2D texDiffuse; // screen display texture
layout (binding = 1) uniform RSDKBuffer {
    vec2 pixelSize;   // internal game resolution (usually 424x240 or smth)
    vec2 textureSize; // size of the internal framebuffer texture
    vec2 viewSize;    // window viewport size
#if RETRO_REV02  // if RETRO_REV02 is defined it assumes the engine is plus/rev02 RSDKv5, else it assumes pre-plus/Rev01 RSDKv5
    float screenDim; // screen dimming percent
#endif
};


void main()
{
    vec3 yuv;
    yuv.r  = texture(texDiffuse, ex_UV).r;
    yuv.gb = texture(texDiffuse, clamp(ex_UV / vec2(2.0), 0.0, 0.499)).gb;
    yuv -= vec3(16.0 / 256.0, .5, .5);

    out_frag.r = 1.164 * yuv.r + 1.596 * yuv.b;
    out_frag.g = 1.164 * yuv.r - 0.392 * yuv.g - 0.813 * yuv.b;
    out_frag.b = 1.164 * yuv.r + 2.017 * yuv.g;
#if RETRO_REV02 
    out_frag.rgb *= screenDim;
#endif
}