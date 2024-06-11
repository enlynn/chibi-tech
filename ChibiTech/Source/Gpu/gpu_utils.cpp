#include "gpu_utils.h"

D3D12_HEAP_PROPERTIES GetHeapProperties(D3D12_HEAP_TYPE Type)
{
    D3D12_HEAP_PROPERTIES Result = {};
    Result.Type                  = Type;
    Result.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    Result.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    Result.CreationNodeMask      = 1;
    Result.VisibleNodeMask       = 1;
    return Result;
}

D3D12_RESOURCE_DESC
GetResourceDesc(
    D3D12_RESOURCE_DIMENSION Dimension,
    UINT64                   Alignment,
    UINT64                   Width,
    UINT                     Height,
    UINT16                   DepthOrArraySize,
    UINT16                   MipLevels,
    DXGI_FORMAT              Format,
    UINT                     SampleCount,
    UINT                     SampleQuality,
    D3D12_TEXTURE_LAYOUT     Layout,
    D3D12_RESOURCE_FLAGS     Flags)
{
    D3D12_RESOURCE_DESC Result = {};
    Result.Dimension           = Dimension;
    Result.Alignment           = Alignment;
    Result.Width               = Width;
    Result.Height              = Height;
    Result.DepthOrArraySize    = DepthOrArraySize;
    Result.MipLevels           = MipLevels;
    Result.Format              = Format;
    Result.SampleDesc.Count    = SampleCount;
    Result.SampleDesc.Quality  = SampleQuality;
    Result.Layout              = Layout;
    Result.Flags               = Flags;
    return Result;
}

D3D12_RESOURCE_DESC
GetBufferResourceDesc(
    D3D12_RESOURCE_ALLOCATION_INFO& ResAllocInfo,
    D3D12_RESOURCE_FLAGS            Flags)
{
    return GetResourceDesc(D3D12_RESOURCE_DIMENSION_BUFFER, ResAllocInfo.Alignment, ResAllocInfo.SizeInBytes,
                            1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, Flags);
}

D3D12_RESOURCE_DESC
GetBufferResourceDesc(
    u64                  Width,
    D3D12_RESOURCE_FLAGS Flags,
    u64                  Alignment)
{
    return GetResourceDesc(D3D12_RESOURCE_DIMENSION_BUFFER, Alignment, Width, 1, 1, 1,
                            DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, Flags);
}

D3D12_RESOURCE_DESC GetTex2DDesc(
    DXGI_FORMAT          Format,
    u64                  Width,
    u32                  Height,
    u16                  ArraySize,
    u16                  MipLevels,
    u32                  SampleCount,
    u32                  SampleQuality,
    D3D12_RESOURCE_FLAGS Flags,
    D3D12_TEXTURE_LAYOUT Layout,
    u64                  Alignment)
{
    return GetResourceDesc(D3D12_RESOURCE_DIMENSION_TEXTURE2D, Alignment, Width, Height, ArraySize,
                           MipLevels, Format, SampleCount, SampleQuality, Layout, Flags);
}