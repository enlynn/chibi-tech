struct PixelInput
{
    float4 Position : SV_POSITION;
    float3 Color    : COLOR0;
};

float4 main(PixelInput Input) : SV_TARGET
{
    return float4(Input.Color, 1.0f);
}