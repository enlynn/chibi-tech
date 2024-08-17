#include "stdafx.h"
#include "GpuRenderTexture.h"

#include "GpuState.h"
#include "GpuUtils.h"

namespace {
    
}

GpuRenderTexture GpuRenderTextureManager::createFramebufferImage(const GpuRenderTextureInfo& tInfo)
{
    u32 SampleCount = 1;
    u32 SampleQuality = 0;
    if (tInfo.mUseMSAA)
    {
        DXGI_SAMPLE_DESC Samples = mDevice->getMultisampleQualityLevels(tInfo.mFormat);
        SampleCount = Samples.Count;
        SampleQuality = Samples.Quality;
    }

    // If this is
    const DXGI_FORMAT framebufferFormat = (tInfo.mIsDepth) ? tInfo.mDepthFormat : tInfo.mFormat;

    D3D12_RESOURCE_DESC imageDesc = getTex2DDesc(framebufferFormat, tInfo.mWidth, tInfo.mHeight, 1, 1, SampleCount, SampleQuality);

    // Allow UAV in case a Renderpass wants to read from the Image
    imageDesc.Flags = tInfo.mIsDepth ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : 
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET|D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = imageDesc.Format;

    if (tInfo.mIsDepth)
    {
        clearValue.DepthStencil = { 1.0f, 0 };
    }
    else
    {
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;
    }

    GpuResource imageResource = GpuResource(*mDevice, imageDesc, clearValue);
    return GpuTexture(mDevice.get(), imageResource);
}
