
struct vertex_output
{
    float4 f4Position : SV_POSITION;
    float3 f3Color    : COLOR0;
};

struct simple_vertex
{
    float3 f3Position;
    float3 f3Normal;
    float2 f2Tex0;
};

struct VertexDrawConstants
{
    uint uVertexOffset;
    uint uVertexBufferIndex;
    uint uMeshDataIndex;
};

struct per_mesh_data
{
    float4x4 Projection; // Ideally this would go in a "global" constant  buffer. For now, this should be fine.
    float4x4 View;
    float4x4 Transform;  // Rotation-Scale-Translation Ops
};

#define BufferSpace  space0
#define PerMeshSpace space1

ByteAddressBuffer                     BufferTable[]        : register(t0, BufferSpace);
StructuredBuffer<per_mesh_data>       PerMeshData          : register(t1, PerMeshSpace);

ConstantBuffer<VertexDrawConstants> cVertexDrawConstants : register(b0, space0);

simple_vertex ExtractVertex(uint uVertexId, VertexDrawConstants DrawConstants)
{
    simple_vertex Result = BufferTable[DrawConstants.uVertexBufferIndex].Load<simple_vertex>((DrawConstants.uVertexOffset + uVertexId) * sizeof(simple_vertex));
    return Result;
}

vertex_output main(uint uVertexId : SV_VertexID)
{
    simple_vertex mVertex = ExtractVertex(uVertexId, cVertexDrawConstants);

    per_mesh_data MeshData = PerMeshData[cVertexDrawConstants.uMeshDataIndex];

    float4 Position = mul(MeshData.Transform, float4(mVertex.f3Position, 1.0f));
    Position = mul(MeshData.View, Position);
    Position = mul(MeshData.Projection, Position);
    
    vertex_output Result;
    Result.f4Position = Position;
    Result.f3Color    = float3(mVertex.f2Tex0, 0.0f);
    
    return Result;
}