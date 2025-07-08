// =======================
// VARIABLES
// =======================

cbuffer RSDKBuffer : register(b0)
{
	float2 pixelSize;		// internal game resolution (usually 424x240 or smth)
	float2 textureSize;		// size of the internal framebuffer texture
	float2 viewSize;		// window viewport size

#if defined(RETRO_REV02)	// if RETRO_REV02 is defined it assumes the engine is plus/rev02 RSDKv5, else it assumes pre-plus/Rev01 RSDKv5
	float2 screenDim;		// screen dimming percent
#endif
};

Texture2D texDiffuse : register(t0);    	// screen display texture
SamplerState sampDiffuse : register(s0); 	// screen display sampler


// =======================
// STRUCTS
// =======================

struct VertexInput
{
    float3 pos      : SV_POSITION;
    float2 tex      : TEXCOORD;
};

struct VertexOutput
{
    float4 pos      : SV_POSITION;
    float4 tex      : TEXCOORD;
};

struct PixelInput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

// =======================
// ENTRY POINTS
// =======================

VertexOutput VSMain(VertexInput input)
{
	VertexOutput output;
	
	output.pos      = float4(input.pos.xyz, 1.0);
	output.tex      = float4(input.tex.xy, 0.0, 0.0);
	
	return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    float2 viewScale = frac((1.0 / pixelSize) * viewSize) - 0.01;

    // if viewSize is an integer scale of pixelSize (within a small margin of error)
    if (viewScale.x < 0 && viewScale.y < 0) {
        // just get the pixel at this fragment with no filtering
#if defined(RETRO_REV02) 
        return texDiffuse.Sample(sampDiffuse, input.tex) * screenDim.x;
#else
        return texDiffuse.Sample(sampDiffuse, input.tex);
#endif
    }

    // otherwise, it's not pixel perfect... do a bit of pixel filtering
    // we have to do it manually here since the engine samples this shader using the "point" filter, rather than "linear"

    float2 adjacent;
    adjacent.x = abs(ddx(input.tex.x));
    adjacent.y = abs(ddy(input.tex.y));

    float4 texPos;
    texPos.zw = adjacent.yx * 0.500501 + input.tex.yx;
    texPos.xy = -adjacent.xy * 0.500501 + input.tex.xy;

    float2 texSize  = 1.0 / textureSize.yx;
    float2 texCoord = clamp(texSize.xy * round(input.tex.yx / texSize.xy), texPos.yx, texPos.zw);
    
    float4 blendFactor;
    blendFactor.xy = -texPos.xy +  texCoord.yx;
    blendFactor.zw =  texPos.zw + -texCoord.xy;

    float strength = adjacent.x * adjacent.y * 0.500501 * 2.002;

    float4 filteredColor = 
        ((blendFactor.x * blendFactor.y) / strength) * texDiffuse.Sample(sampDiffuse, texPos.xy) + 
        ((blendFactor.z * blendFactor.w) / strength) * texDiffuse.Sample(sampDiffuse, texPos.wz) + 
        ((blendFactor.z * blendFactor.x) / strength) * texDiffuse.Sample(sampDiffuse, texPos.xz) +
        ((blendFactor.w * blendFactor.y) / strength) * texDiffuse.Sample(sampDiffuse, texPos.wy); 
    
#if defined(RETRO_REV02) 
    filteredColor *= screenDim.x;
#endif
    return filteredColor;
}