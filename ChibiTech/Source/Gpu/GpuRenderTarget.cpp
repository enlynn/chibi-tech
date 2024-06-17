#include "GpuRenderTarget.h"
#include "GpuResource.h"

#include <platform/platform.h>

void GpuRenderTarget::attachTexture(AttachmentPoint Slot, GpuTexture* Texture)
{
#if defined(DEBUG_BUILD)
    if (mAttachments[u32(Slot)] != nullptr)
    {
        LogWarn("Attaching Texture to Slot (%d), but texture already exists. Replacing old texture.", u32(Slot));
    }

    D3D12_RESOURCE_DESC TexDesc = Texture->GetResourceDesc();
    if (TexDesc.Width != mWidth || TexDesc.Height != mHeight)
    {
        //LogWarn("Attaching Texture to RenderTarget (), Texture dimensions do not match. Render Target Dim (%d, %d), Texture Dims (%d, %d).",
        //    mWidth, mHeight, TexDesc.Width, TexDesc.Height);

        mWidth  = (u32)TexDesc.Width;
        mHeight = (u32)TexDesc.Height;
    }
#endif

    mAttachments[u32(Slot)] = Texture;
}

void GpuRenderTarget::detachTexture(AttachmentPoint Slot)
{
    mAttachments[u32(Slot)] = nullptr;
}

// Resets the state of the render target (all attached textures are removed)
void GpuRenderTarget::reset()
{
    ForRange(int, i, int(AttachmentPoint::Count))
    {
        mAttachments[i] = nullptr;
    }
}

D3D12_VIEWPORT
GpuRenderTarget::getViewport(float2 Scale, float2 Bias, f32 MinDepth, f32 MaxDepth)
{
    u32 Width  = 1;
    u32 Height = 1;

    ForRange(int, i, int(AttachmentPoint::DepthStencil))
    {
        if (mAttachments[i])
        {
            D3D12_RESOURCE_DESC Desc = mAttachments[i]->getResourceDesc();
            Width                    = std::max(static_cast<u64>(Width),  Desc.Width);
            Height                   = std::max(static_cast<u32>(Height), Desc.Height);
        }
    }

    D3D12_VIEWPORT Viewport = {
        .TopLeftX = Width  * Bias.X,
        .TopLeftY = Height * Bias.Y,
        .Width    = Width  * Scale.X,
        .Height   = Height * Scale.Y,
        .MinDepth = MinDepth,
        .MaxDepth = MaxDepth,
    };

    return Viewport;
}

D3D12_RT_FORMAT_ARRAY
GpuRenderTarget::getAttachmentFormats()
{
    D3D12_RT_FORMAT_ARRAY Formats = {};

    ForRange(int, i, int(AttachmentPoint::DepthStencil))
    {
        D3D12_RESOURCE_DESC Desc = mAttachments[i]->getResourceDesc();

        Formats.RTFormats[Formats.NumRenderTargets] = Desc.Format;
        Formats.NumRenderTargets += 1;
    }

    return Formats;
}

const GpuTexture*
GpuRenderTarget::getTexture(AttachmentPoint Slot)
{
    return mAttachments[int(Slot)];
}

DXGI_FORMAT GpuRenderTarget::getDepthFormat()
{
    DXGI_FORMAT Result = DXGI_FORMAT_UNKNOWN;

    const GpuTexture* DepthTexture = mAttachments[int(AttachmentPoint::DepthStencil)];
    if (DepthTexture != nullptr)
    {
        D3D12_RESOURCE_DESC Desc = DepthTexture->getResourceDesc();
        Result = Desc.Format;
    }
    return Result;
}

