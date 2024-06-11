
struct VertexOutput
{
    float4 f4Position : SV_POSITION;
    float3 f3Color    : COLOR0;
};

struct Vertex
{
    float3 f3Position;
    float3 f3Color;
};

struct VertexDrawConstants
{
    uint uVertexOffset;
    uint uVertexBufferIndex;
    uint uMeshDataIndex;
};

ByteAddressBuffer                   BufferTable[]        : register(t0, space0);
ConstantBuffer<VertexDrawConstants> cVertexDrawConstants : register(b0, space0);

Vertex ExtractVertex(uint uVertexId, VertexDrawConstants DrawConstants)
{
    Vertex result = BufferTable[DrawConstants.uVertexBufferIndex].Load<Vertex>((DrawConstants.uVertexOffset + uVertexId) * sizeof(Vertex));
    return result;
}

VertexOutput main(uint uVertexId : SV_VertexID)
{
    Vertex mVertex = ExtractVertex(uVertexId, cVertexDrawConstants);
    
    VertexOutput Result;
    Result.f4Position = float4(mVertex.f3Position, 1.0f);
    Result.f3Color    = mVertex.f3Color;
    
    return Result;
}