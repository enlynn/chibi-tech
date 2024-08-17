#pragma once

#include <unordered_map>

#include <types.h>
#include "D3D12Common.h"
#include "GpuTexture.h"

#include <Util/Hash.h>

class GpuState;
class GpuDevice;

// A render texture is a texture than can be bound as a render target.
using GpuRenderTexture = GpuTexture;

struct GpuRenderTextureInfo
{
    // Leave width/height at zero to match swapchain dimensions
    u32         mWidth{ 0 };
    u32         mHeight{ 0 };
    // If format is "Unknown", then the swapchain format is chosen
    DXGI_FORMAT mFormat{ DXGI_FORMAT_UNKNOWN };
    bool        mIsDepth{ false };
    // If format is "Unknown", depth is enabled, and mIsDepth is true, then the swapchain depth format is chosen
    DXGI_FORMAT mDepthFormat{ DXGI_FORMAT_UNKNOWN };
    // If the Render Device has enabled MSAA and this should be a multisampled target, set to true.
    // Be careful not to mix a non-msaa buffer during msaa render passes!
    bool        mUseMSAA{ true };
};

template<>
struct std::hash<GpuRenderTextureInfo>
{
    size_t operator()(const GpuRenderTextureInfo& rtInfo) const noexcept
    {
        size_t widthHash       = std::hash<uint32_t>{}(rtInfo.mWidth);
        size_t heightHash      = std::hash<uint32_t>{}(rtInfo.mHeight);
        size_t formatHash      = std::hash<uint32_t>{}(rtInfo.mFormat);
        size_t depthFormatHash = std::hash<uint32_t>{}(rtInfo.mDepthFormat);
        size_t isDepthHash     = std::hash<bool>{}(rtInfo.mIsDepth);
        size_t useMSAAHash     = std::hash<bool>{}(rtInfo.mUseMSAA);

        size_t finalHash = widthHash;
        hash_combine_size_t(finalHash, heightHash);
        hash_combine_size_t(finalHash, formatHash);
        hash_combine_size_t(finalHash, depthFormatHash);
        hash_combine_size_t(finalHash, isDepthHash);
        hash_combine_size_t(finalHash, useMSAAHash);

        return finalHash;
    }
};

GpuRenderTextureInfo getRenderTextureInfo(const GpuRenderTexture& tTexture);

class GpuRenderTextureManager 
{
public:
    GpuRenderTextureManager();
    ~GpuRenderTextureManager() { destroy(); }

    void destroy();

    // Retrieves an available RenderTexture based on its info struct. If one is not found,
    // a new render texture is created. 
    GpuRenderTexture getRenderTexture(GpuRenderTextureInfo);

    // Returns the render texture to the in-use frame cache if the texture is still rendering
    // on the gpu. If in-flight, will  be added to the cache when the FrameCache is reset, which
    // guarantees the texture is not in use on the GPU.
    void releaseGpuRenderTexture(GpuRenderTexture);
    
    // Add named render texture. Useful when you want other RenderPasses to read from a render texture
    void addNamedRenderTexture();

    // Clears in-flight and named textures and adds them to the available cache.
    void reset();

    // Clears *all* render textures. This should be called when there is a resize event.
    void clear();

private:
    GpuRenderTexture createFramebufferImage(const GpuRenderTextureInfo& tInfo);

private:
    using TextureMap          = std::unordered_map<GpuRenderTextureInfo, GpuRenderTexture>;
    using TextureNameMap      = std::unordered_map<std::string, GpuRenderTexture>;
    using InFlightTextureList = std::vector<GpuRenderTexture>;

    std::shared_ptr<GpuDevice> mDevice;

    // Cache of unused but available render textures that can be uniquely identified
    // by their info struct.
    TextureMap                 mRenderTextureCache{};
    // Active Render Textures that can be fetched to be re-used or read from.
    // Render Textures will be cleared and added to the cache when the FrameCache is reset.
    TextureNameMap             mNamedRenderTextures{};
    // List of render textures that might be in-use on the GPU, but are no longer
    // needed this frame. Will be returned to the cache from the frame cache is reset.
    InFlightTextureList        mInFlightRenderTextures{};

    DXGI_FORMAT                mSwapchainFormat{};
    DXGI_FORMAT                mDepthFormat{};
    DXGI_SAMPLE_DESC           mMsaaDesc{};
};
