struct PixelInput
{
    float4 f4Position : SV_POSITION;
    float2 f2TexCoord : TEXCOORD;
};

Texture2D<float4> DiffuseTexture       : register(t1, space1);
SamplerState      LinearRepeatSampler  : register(s0, space0);

float4 main(PixelInput IN) : SV_TARGET
{
    return float4(DiffuseTexture.Sample(LinearRepeatSampler, IN.f2TexCoord).rgb, 1.0f);
}