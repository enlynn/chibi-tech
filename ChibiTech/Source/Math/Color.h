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
        struct { f32x2 RG; f32 Pad0; };
        struct { f32 Pad1; f32x2 GB; };
        f32x3 RGB;
    };
};

struct color_rgba
{
    colorspace Colorspace = colorspace::linear;
    union
    {
        struct { f32 X, Y, Z, W;         };
        struct { f32x2 XY;   f32x2 ZW;   };
        struct { f32x3 XYZ;  f32   Pad0; };
        struct { f32   Pad1; f32x3 YZW;  };
        f32x4 RGBA;
    };
};