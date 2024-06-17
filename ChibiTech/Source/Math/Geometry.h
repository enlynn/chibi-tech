#pragma once

#include "Math.h"

#include <vector>

namespace ct {
    struct GeometryVertex
    {
        float3 mPos;
        float3 mNorm;
        float2 mTex;
    };

    struct GeometryCube
    {
        GeometryVertex mVertices[24];
        u16            mIndices[36];
    };

    struct GeometrySphere {
        std::vector<GeometryVertex> mVertices{};
        std::vector<u32>            mIndices{};
    };

    GeometryCube makeCube(f32 tSize = 1.0f, bool tShouldReverseWinding = false, bool tInvertNormals = false);
    GeometrySphere makeSphere(f32 radius = 1.0f, u32 tessellation = 3, bool reverse_winding = false);
}
