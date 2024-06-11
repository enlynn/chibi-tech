#include "Geometry.h"

#include <Platform/Assert.h>

namespace {
    template<typename V, typename I> void
    reverseWinding(I* tIndices, u32 tIndexCount, V* tVertices, u32 tVertexCount)
    {
        for (u32 i = 0; i < tIndexCount; i += 3)
        {
            I _x = tIndices[i];
            I _y = tIndices[i+2];

            tIndices[i]   = _y;
            tIndices[i+2] = _x;
        }

        for (u32 i = 0; i < tVertexCount; ++i)
        {
            tVertices[i].mTex.X = (1.0f - tVertices[i].mTex.X);
        }
    }
}

namespace ct {
    GeometryCube makeCube(f32 tSize, bool tShouldReverseWinding, bool tInvertNormals)
    {
        // 8 edges of cube.
        f32x3 p[8] = {
                {  tSize, tSize, -tSize }, {  tSize, tSize,  tSize }, {  tSize, -tSize,  tSize }, {  tSize, -tSize, -tSize },
                { -tSize, tSize,  tSize }, { -tSize, tSize, -tSize }, { -tSize, -tSize, -tSize }, { -tSize, -tSize,  tSize }
        };

        // 6 face normals
        f32x3 n[6] = { { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
        // 4 unique texture coordinates
        f32x2 t[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

        // Indices for the Vertex positions.
        u16 i[24] = {
                0, 1, 2, 3,  // +X
                4, 5, 6, 7,  // -X
                4, 1, 0, 5,  // +Y
                2, 7, 6, 3,  // -Y
                1, 4, 7, 2,  // +Z
                5, 0, 3, 6   // -Z
        };

        GeometryCube Result = {};
        memset(Result.mVertices, 0, sizeof(Result.mVertices));
        memset(Result.mIndices, 0, sizeof(Result.mIndices));

        if (tInvertNormals)
        {
            for (auto & f : n)  // For each face of the cube.
            {
                f *=  -1.0f;
            }
        }

        u16 vidx = 0;
        u16 iidx = 0;
        for (u16 f = 0; f < 6; ++f )  // For each face of the cube.
        {
            // Four vertices per face.
            Result.mVertices[vidx++] = { p[i[f * 4 + 0]], n[f], t[0] };
            Result.mVertices[vidx++] = { p[i[f * 4 + 1]], n[f], t[1] };
            Result.mVertices[vidx++] = { p[i[f * 4 + 2]], n[f], t[2] };
            Result.mVertices[vidx++] = { p[i[f * 4 + 3]], n[f], t[3] };

            // First triangle.
            Result.mIndices[iidx++] =  f * 4 + 0;
            Result.mIndices[iidx++] =  f * 4 + 1;
            Result.mIndices[iidx++] =  f * 4 + 2;

            // Second triangle.
            Result.mIndices[iidx++] =  f * 4 + 2;
            Result.mIndices[iidx++] =  f * 4 + 3;
            Result.mIndices[iidx++] =  f * 4 + 0;
        }

        if (tShouldReverseWinding)
        {
            reverseWinding(Result.mIndices, _countof(Result.mIndices), Result.mVertices, _countof(Result.mVertices));
        }

        return Result;
    }

    GeometrySphere makeSphere(f32 tRadius, u32 tTessellation, bool tReverseWinding) {
        ASSERT(tTessellation > 3);

        GeometrySphere sphere{};

        u64 verticalSegments   = tTessellation;
        u32 horizontalSegments = tTessellation;
        //u32 horizontalSegments = tessellation * 2;

        // Create rings of sphere.mVertices at progressively higher latitudes.
        for ( u64 i = 0; i <= verticalSegments; i++ )
        {
#if 1
            f32 v = 1 - (f32)i / (f32)verticalSegments;
            f32 latitude = ( (f32)i * F32_PI / (f32)verticalSegments ) - F32_PIDIV2;
            f32 dy  = sinf(latitude);
            f32 dxz = cosf(latitude);
#endif

            // Create a single ring of sphere.mVertices at this latitude.
            for ( u64 j = 0; j <= horizontalSegments; j++ )
            {
#if 1
                f32 u = (f32)j / (f32)horizontalSegments;

                f32 longitude = (f32)j * F32_2PI / (f32)horizontalSegments;
                f32 dx = sinf(longitude);
                f32 dz = cosf(longitude);

                dx *= dxz;
                dz *= dxz;

                f32x3 normal = { dx, dy, dz };
                f32x2 uv0    = { u, v };
                f32x3 pos    = normal * tRadius; //v3_mulf(normal, radius);
#else
            f32 xSegment = (f32)j / (f32)horizontalSegments;
            f32 ySegment = (f32)i / (f32)verticalSegments;
            f32 xPos = cosf(xSegment * 2.0f * MM_PI) * sinf(ySegment * MM_PI);
            f32 yPos = cosf(ySegment * MM_PI);
            f32 zPos = sinf(xSegment * 2.0f * MM_PI) * sinf(ySegment * MM_PI);

            v3 normal = { xPos, yPos, zPos };
            v2 uv0    = { xSegment, ySegment };
            v3 pos    = v3_mulf(normal, radius);
#endif

                GeometryVertex vertex = {};
                vertex.mPos = pos;
                vertex.mNorm = normal;
                vertex.mTex = uv0;
                sphere.mVertices.push_back(vertex);
            }
        }

        // Fill the index buffer with triangles joining each pair of latitude rings.
        u32 stride = horizontalSegments + 1;

#if 1
        for (u32 i = 0; i < verticalSegments; i++ )
        {
            for (u32 j = 0; j <= horizontalSegments; j++)
            {
                u32 nextI = i + 1;
                u32 nextJ = ( j + 1 ) % stride;

                sphere.mIndices.push_back(i * stride + nextJ);
                sphere.mIndices.push_back(nextI * stride + j);
                sphere.mIndices.push_back(i * stride + j);

                sphere.mIndices.push_back(nextI * stride + nextJ);
                sphere.mIndices.push_back(nextI * stride + j);
                sphere.mIndices.push_back(i * stride + nextJ);
            }
        }
#else
        bool oddRow = false;
        for (unsigned int y = 0; y < verticalSegments; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= horizontalSegments; ++x)
                {
                    arrput(indices, y       * (horizontalSegments + 1) + x);
                    arrput(indices, (y + 1) * (horizontalSegments + 1) + x);
                }
            }
            else
            {
                for (int x = horizontalSegments; x >= 0; --x)
                {
                    arrput(indices, (y + 1) * (horizontalSegments + 1) + x);
                    arrput(indices, y       * (horizontalSegments + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
#endif

        if (tReverseWinding)
        {
            reverseWinding(sphere.mIndices.data(), sphere.mIndices.size(), sphere.mVertices.data(), (u32)sphere.mVertices.size());
        }

        return  sphere;
    }
}