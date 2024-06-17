#pragma once

#include <types.h>
#include "../Math/Math.h"

#include "D3D12Common.h"
#include "GpuTexture.h"

class GpuResource;

enum class AttachmentPoint : u8
{
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    DepthStencil,
    Count,
};

class GpuRenderTarget
{
public:
    GpuRenderTarget() = default;
    GpuRenderTarget(u32 Width, u32 Height) { mWidth = Width; mHeight = Height; }

    void attachTexture(AttachmentPoint Slot, GpuTexture* Texture);
    void detachTexture(AttachmentPoint Slot);

    // Resets the state of the render target (all attached textures are removed)
    void reset();

    // resize a RenderTarget based on the width/height. This is done to keep all
    // attached textures the same dimensions.
    //void resize(u32 Width, u32 Height);

    const GpuTexture* getTexture(AttachmentPoint Slot);

    // Returns a packed array of the image formats for the textures assigned to this
    // Render Target. Particularly useful for creating PSOs.
    //
    // TODO(enlynn): Determine if PSO require the RT formats to be packed or sparse.
    D3D12_RT_FORMAT_ARRAY getAttachmentFormats();
    // Gets the depth format for this RenderTarget. If there is not a Depth target
    // attached, then DXGI_FORMAT_UNKNOWN is returned.
    DXGI_FORMAT           getDepthFormat();

    // Get the viewport for this render target
    // Scale & bias parameters can be used to specify a split screen viewport
    // (bias parameter is normalized in the range [0...1]). 
    //
    // By default, fullscreen is return.
    D3D12_VIEWPORT getViewport(float2 Scale = {1.0f, 1.0f},
                               float2 Bias  = {0.0f, 0.0f},
                               f32 MinDepth = 0.0f, f32 MaxDepth = 1.0f);

private:
    // TODO(enlynn): Ideally, we would not be storing the Resource directly, but instead
    // be using a texture_id or something equivalent. This is just an in-place solution'
    // until textures are implemented.
    GpuTexture*  mAttachments[int(AttachmentPoint::Count)] = {};
    // The expected width and height of the attached textures
    u32           mWidth  = 0;
    u32           mHeight = 0;
};
