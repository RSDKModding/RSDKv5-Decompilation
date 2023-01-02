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
    out_frag = texture(texDiffuse, ex_UV);
#if RETRO_REV02 
    out_frag.rgb *= screenDim;
#endif
}