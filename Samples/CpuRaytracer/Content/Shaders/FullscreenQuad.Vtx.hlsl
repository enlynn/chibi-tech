
struct VertexOutput
{
    float4 f4Position : SV_POSITION;
    float2 f2TexCoord : TEXCOORD;
};

VertexOutput main(uint uVertexId : SV_VertexID)
{
    VertexOutput Result;
    Result.f2TexCoord = float2(uVertexId & 1, uVertexId >> 1);
    Result.f4Position = float4((Result.f2TexCoord.x - 0.5f) * 2.0f, -(Result.f2TexCoord.y - 0.5f) * 2.0f, 0.0f, 1.0f);
    
    return Result;
}