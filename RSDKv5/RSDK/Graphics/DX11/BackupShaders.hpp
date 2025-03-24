const char backupHLSL[] =
R"(
cbuffer RSDKBuffer : register(b0)
{
    float2 pixelSize;
    float2 textureSize;
    float2 viewSize;

#if defined(RETRO_REV02)
    float2 screenDim;
#endif
};

Texture2D texDiffuse : register(t0);
SamplerState sampDiffuse : register(s0);

struct VertexInput {
    float3 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

struct VertexOutput {
    float4 pos : SV_POSITION;
    float4 tex : TEXCOORD;
};

struct PixelInput {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    output.pos = float4(input.pos.xyz, 1.0);
    output.tex = float4(input.tex.xy, 0.0, 0.0);

    return output;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    return texDiffuse.Sample(sampDiffuse, input.tex);
}
)";
