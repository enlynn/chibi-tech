#include "gpu_render_target.h"
#include "gpu_resource.h"

#include <platform/platform.h>

void gpu_render_target::AttachTexture(attachment_point Slot, gpu_texture* Texture)
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

void gpu_render_target::DetachTexture(attachment_point Slot)
{
    mAttachments[u32(Slot)] = nullptr;
}

// Resets the state of the render target (all attached textures are removed)
void gpu_render_target::Reset()
{
    ForRange(int, i, int(attachment_point::count))
    {
        mAttachments[i] = nullptr;
    }
}

D3D12_VIEWPORT
gpu_render_target::GetViewport(f32x2 Scale, f32x2 Bias, f32 MinDepth, f32 MaxDepth)
{
    u32 Width  = 1;
    u32 Height = 1;

    ForRange(int, i, int(attachment_point::depth_stencil))
    {
        if (mAttachments[i])
        {
            D3D12_RESOURCE_DESC Desc = mAttachments[i]->GetResourceDesc();
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
gpu_render_target::GetAttachmentFormats()
{
    D3D12_RT_FORMAT_ARRAY Formats = {};

    ForRange(int, i, int(attachment_point::depth_stencil))
    {
        D3D12_RESOURCE_DESC Desc = mAttachments[i]->GetResourceDesc();

        Formats.RTFormats[Formats.NumRenderTargets] = Desc.Format;
        Formats.NumRenderTargets += 1;
    }

    return Formats;
}

const gpu_texture*
gpu_render_target::GetTexture(attachment_point Slot)
{
    return mAttachments[int(Slot)];
}

DXGI_FORMAT gpu_render_target::GetDepthFormat()
{
    DXGI_FORMAT Result = DXGI_FORMAT_UNKNOWN;

    const gpu_texture* DepthTexture = mAttachments[int(attachment_point::depth_stencil)];
    if (DepthTexture != nullptr)
    {
        D3D12_RESOURCE_DESC Desc = DepthTexture->GetResourceDesc();
        Result = Desc.Format;
    }
    return Result;
}

