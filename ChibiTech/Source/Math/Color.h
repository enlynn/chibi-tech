//
// Created by enlynn on 11/6/2023.
//
#pragma once

#include "Math.h"

enum class colorspace : u8
{
    unknown,
    linear,
};

struct color_rgb
{
    colorspace Colorspace = colorspace::linear;
    union
    {
        struct { f32 R, G, B;        };
        struct { float2 RG; f32 Pad0; };
        struct { f32 Pad1; float2 GB; };
        float3 RGB;
    };
};

struct color_rgba
{
    colorspace Colorspace = colorspace::linear;
    union
    {
        struct { f32 X, Y, Z, W;         };
        struct { float2 XY;   float2 ZW;   };
        struct { float3 XYZ;  f32   Pad0; };
        struct { f32   Pad1; float3 YZW;  };
        float4 RGBA;
    };
};