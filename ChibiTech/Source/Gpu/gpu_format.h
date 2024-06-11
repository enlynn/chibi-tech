#pragma once

#include "D3D12Common.h"

enum class gpu_format
{
    // image formats
    rgb8_unorm,

    // depth formats
};

inline DXGI_FORMAT to_dxgi_format(gpu_format Format)
{
    DXGI_FORMAT Result = DXGI_FORMAT_UNKNOWN;
    switch (Format)
    {
        case gpu_format::rgb8_unorm:
        default: UNIMPLEMENTED;
    }
    return Result;
}
