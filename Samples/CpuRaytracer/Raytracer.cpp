#include "Raytracer.h"

#include <Platform/Console.h>

//
// Design decision (for now):
//
// Geometry Manager:
// - Primitives:
// -    Spheres
// -    Tris
// -    Boxes
// -    Quads
// -    Planes
//
// Bottom Level Acceleration Structure (BLAS)
// - Builds a spatial heirarchy around the geometry.
// - However, for geometry like spheres, boxes, and planes, the geometry is really simply.
//   would only need a simple struct to encase the data. Quads and Tries (where there is a
//   lot of geometry), we'll want a complex structure.
// - Types of BLAS:
// -    SimpleBLAS
// -    TriBLAS
// -    QuadBLAS
// - Perhaps something like:
// -    std:::unique_ptr<> blas = GeometryManager::buildBLAS(...)
// -    blas->intersect(...);
// - BLAS's are intersected *infrequently* compared to their counterpart, TLAS.
// - Internally, this might be represented as:
// -    struct BlasId { u8 blasType; u64 id; };
// -        tlas->intersect() ... blas->intersect()
// - If doing the pointer approach, how do we rebuild the tlas when the camera move?
// - Internally, we could do:
// -    SparseStorage<std::unique_ptr<Blas>> -> container.iterator() -> build each object
//
//
//
// Top Level Acceleration Structure (TLAS)
// - Stores a list of Object within the Accel structure
//
//
//
//
//
//

struct Ray {
    Ray(float3 tOrigin, float3 tDirection) : mOrigin(tOrigin), mDirection(tDirection) {}

    [[nodiscard]] float3 at(f32 t) const {
        return mOrigin + (t * mDirection);
    }

    float3 mOrigin{};
    float3 mDirection{};
};

f32 intersectSphere(const float3& tCenter, f32 tRadius, const Ray& tRay) {
    // Given the equation for a sphere:
    // (Cx - x)^2 + (Cy - y)^2 + (Cy - y)^2 = r^2 , where (Cx, Cy, Cz) is the sphere origin and r is the radius
    // we can re-write the equation like so:
    // (C - P) dot (C - P) = r^2, where C is the sphere origin and P = (x, y, z)

    // Given the equation for a ray:
    // P(t) = Q + t * d, where Q is the ray origin, d is the ray direction

    // Substitute "P" with the equation of the ray
    // (C - P(t)) dot (C - P(t)) = r^2

    // Expand and simplify:
    // (d dot d) * t^2 - (2 * (d dot (C - Q))) * t + (C - Q) dot (C - Q) - r^2 = 0

    // As this is a quadratic function, let the terms be:
    // a = (d dot d)
    // b = -(2 * (d dot (C - Q)))
    // c = (C - Q) dot (C - Q) - r^2

    // However, when considering the quadratic equation, we can factor in the -2 in the b coefficient like so:
    // h = (d dot (C - Q))
    // b = -2 * h
    // determinant: (-2h)^2 - 4ac -> 4*h^2 - 4ac
    // since this is in a sqrt(), can factor it out the 4:
    // -(-2h) +- 2sqrt(h^2 - 4ac) / 2a
    // Simplify, and the quadratic becomes:
    // (h +- sqrt(h^2 - 4ac)) / a

    // C - Q
    float3 rayToCenter = tCenter - tRay.mOrigin;

    // quadratic coefficients: a * t^2 + b * t + c
    f32 coeffA = tRay.mDirection.lengthSq(); //dot(tRay.mDirection, tRay.mDirection);

    //f32 coeffB = -2.0f * dot(tRay.mDirection, rayToCenter);
    f32 h = dot(tRay.mDirection, rayToCenter);

    f32 coeffC = rayToCenter.lengthSq() - tRadius * tRadius;

    // solve for intersection using the quadratic formula
    // the discriminant (d) can be used to determine intersections
    // d > 0 -> 2 intersections
    // d = 0 -> 1 intersection
    // d < 0 -> no intersections
    //f32 d = coeffB * coeffB - 4.0f * coeffA * coeffC;
    f32 d = h * h - coeffA * coeffC;

    // TODO(enlynn): When following Raytracing in a Weekend, my solution seems to diverge a bit from the book, and
    // I don't understand why. It is expected to return a mostly blue circle, while I return a UV gradient with the
    // equation (h - sqrt(d)) / a. Expected result is > 0, but I  get < 0.
    //
    // Need to keep an eye out for bugs related to intersections with in-front vs. behind the camera.

    // Return true for any intersection
    if (d < 0) {
        return 1;
    }
    else {
        return (h - sqrt(d)) / coeffA;
    }
}

float4 colorPixel(const Ray& ray) {
    // See if we intersect the sphere
    f32 closestT = intersectSphere(float3{0.0f, 0.0f, -1.0f}, 0.5f, ray);
    if (closestT < 0.0f) {
        // The normal for a sphere can simply be computed by finding the vector from the center to the intersection point
        float3 normal = ray.at(closestT) - float3{0,0,-1};
        normal.norm();

        float4 color{};
        color.XYZ = 0.5f * float3{normal.X + 1.0f, normal.Y + 1.0f, normal.Z + 1.0f};
        color.W   = 1.0f;
        return color;
    }

    // Color the "skybox"
    float3 unitDirection = ray.mDirection.getNorm();
    float t = 0.5f * (unitDirection.Y + 1.0f);

    float4 color{};
    color.XYZ = ((1.0f - t) * cFloat3One) + (t * float3{0.5f, 0.7f, 1.0f});
    color.W   = 1.0f;
    return color;
}

void raytracerWork(RaytracerWork tWork) {
    const RaytracerState* state = tWork.mState;

    for (int i = 0; i < tWork.mImageWidth; i++) {
        float3 pixelCenter  = state->mPixel00Loc + (f32(i) * state->mPixelDeltaU) + (f32(tWork.mImageHeightIndex) * state->mPixelDeltaV);
        float3 rayDirection = pixelCenter - state->mCameraOrigin;

        Ray ray(pixelCenter, rayDirection);
        tWork.mImageWriteLocation[i] = colorPixel(ray);

        // Progressive rendering:
        // f32 weight = 1.0f / (NumRenderedFrames + 1.0f)
        // float4 accumAverage = oldColor * (1.0f - weight) + newColor * weight;
    }

    //t::console::info("Finished row: %d %lf", tWork.mImageHeightIndex, (f32)tWork.mImageHeightIndex / (f32)(tWork.mImageHeight - 1));
}


// Setup for the read-only Raytracer state
//
RaytracerState::RaytracerState(RaytracerInfo tInfo) {
    mImageWidth   = tInfo.mImageWidth;
    mCameraOrigin = tInfo.mCameraOrigin;

    mImageHeight = size_t(f32(tInfo.mImageWidth) / tInfo.mAspectRatio);
    mImageHeight = mImageWidth < 1 ? 1 : mImageHeight; // prevent a height of 0

    f32 viewportWidth = tInfo.mViewportHeight * (f32(mImageWidth) / f32(mImageHeight));

    mViewportU = float3{viewportWidth,                   0.0f, 0.0f};
    mViewportV = float3{         0.0f, -tInfo.mViewportHeight, 0.0f};

    mPixelDeltaU = mViewportU / f32(mImageWidth);
    mPixelDeltaV = mViewportV / f32(mImageHeight);

    auto viewportUpperLeft = mCameraOrigin - float3{0.0f, 0.0f, tInfo.mFocalLength} - (mViewportU / 2.0f) - (mViewportV / 2.0f);
    mPixel00Loc = viewportUpperLeft + 0.5f * (mPixelDeltaU + mPixelDeltaV);
}