#pragma once

#include <Types.h>

#include <Math/Math.h>

class RaytracerState;
struct RaytracerWork {
    size_t                mImageHeight;
    size_t                mImageWidth;
    size_t                mImageHeightIndex;
    float4*               mImageWriteLocation;
    const RaytracerState* mState;
};


struct RaytracerInfo {
    // Image sizing
    size_t mImageWidth{};
    f32    mAspectRatio{1.0f};

    // Camera settings
    f32    mFocalLength{1.0f};
    f32    mViewportHeight{2.0f};
    float3 mCameraOrigin{0.0f, 0.0f, 0.0f};
};

class RaytracerState {
public:
    explicit RaytracerState(RaytracerInfo tInfo);

    // Image
    size_t mImageWidth;
    size_t mImageHeight;

    // Camera/Viewport
    float3 mCameraOrigin;
    float3 mViewportU;   // Vector going from Left to Right along the viewport
    float3 mViewportV;   // Vector going from Top to Bottom along the viewport
    float3 mPixelDeltaU; // Horizontal Vector going from Pixel to Pixel
    float3 mPixelDeltaV; // Vertical Vector going from Pixel to Pixel
    float3 mPixel00Loc;  // Upper Left Location of the pixel
};

void raytracerWork(RaytracerWork tWork);