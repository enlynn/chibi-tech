#pragma once

#include "D3D12Common.h"

D3D12_HEAP_PROPERTIES getHeapProperties(D3D12_HEAP_TYPE Type = D3D12_HEAP_TYPE_DEFAULT);

D3D12_RESOURCE_DESC getResourceDesc(
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
    D3D12_RESOURCE_FLAGS     Flags);

D3D12_RESOURCE_DESC getBufferResourceDesc(
    D3D12_RESOURCE_ALLOCATION_INFO& ResAllocInfo,
    D3D12_RESOURCE_FLAGS            Flags = D3D12_RESOURCE_FLAG_NONE);

D3D12_RESOURCE_DESC getBufferResourceDesc(
    u64                  Width,
    D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE,
    u64                  Alignment = 0);

D3D12_RESOURCE_DESC getTex2DDesc(
    DXGI_FORMAT          Format,
    u64                  Width,
    u32                  Height,
    u16                  ArraySize     = 1,
    u16                  MipLevels     = 0,
    u32                  SampleCount   = 1,
    u32                  SampleQuality = 0,
    D3D12_RESOURCE_FLAGS Flags         = D3D12_RESOURCE_FLAG_NONE,
    D3D12_TEXTURE_LAYOUT Layout        = D3D12_TEXTURE_LAYOUT_UNKNOWN,
    u64                  Alignment     = 0);