#pragma once

#include "D3D12Common.h"

enum class GpuFormat
{
    // image formats
    Rgb8Unorm,

    // depth formats
};

inline DXGI_FORMAT toDxgiFormat(GpuFormat Format)
{
    DXGI_FORMAT Result = DXGI_FORMAT_UNKNOWN;
    switch (Format)
    {
        case GpuFormat::Rgb8Unorm:
        default: UNIMPLEMENTED;
    }
    return Result;
}
