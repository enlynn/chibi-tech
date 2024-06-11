#pragma once

#include "gpu_resource.h"
#include "gpu_descriptor_allocator.h"

struct gpu_frame_cache;

class gpu_texture
{
public:
    gpu_texture() = default;
    gpu_texture(gpu_frame_cache* FrameCache, gpu_resource Resource);

    void ReleaseUnsafe(gpu_frame_cache* FrameCache);

    void Resize(gpu_frame_cache* FrameCache, u32 Width, u32 Height);
    void CreateViews(gpu_frame_cache* FrameCache);

    D3D12_RESOURCE_DESC GetResourceDesc()     const { return mResource.GetResourceDesc(); }
    const gpu_resource* GetResource()         const { return &mResource;                  }
    cpu_descriptor      GetRenderTargetView() const { return mRTV;                        }
    cpu_descriptor      GetDepthStencilView() const { return mDSV;                        }

    bool CheckSRVSupport()
    {
        return mResource.CheckFormatSupport(D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
    }

    bool CheckRTVSupport()
    {
        return mResource.CheckFormatSupport(D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
    }

    bool CheckUAVSupport()
    {
        return mResource.CheckFormatSupport(D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
               mResource.CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) &&
               mResource.CheckFormatSupport(D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);
    }

    bool CheckDSVSupport()
    {
        return mResource.CheckFormatSupport(D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
    }

    static inline bool IsUAVCompatibleFormat(DXGI_FORMAT Format);
    static inline bool IsSRGBFormat(DXGI_FORMAT Format);
    static inline bool IsBGRFormat(DXGI_FORMAT Format);
    static inline bool IsDepthFormat(DXGI_FORMAT Format);

    // Return a typeless format from the given format.
    static inline DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT Format);
    // Return an sRGB format in the same format family.
    static inline DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT Format);
    static inline DXGI_FORMAT GetUAVCompatibleFormat(DXGI_FORMAT Format);

    // TODO(enlynn): bitsperpixel

private:
    gpu_resource mResource = {};

    //
    // Depending on the type of texture access:
    // - Render Target
    // - Depth Stencil
    // - Shader Resource
    // - UnorderedAccess
    //
    cpu_descriptor mRTV = {};
    cpu_descriptor mDSV = {};
    cpu_descriptor mSRV = {};
    cpu_descriptor mUAV = {};
};

inline bool gpu_texture::IsUAVCompatibleFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT: // Intentional fallthrough
        case DXGI_FORMAT_R32G32B32A32_UINT:  // Intentional fallthrough
        case DXGI_FORMAT_R32G32B32A32_SINT:  // Intentional fallthrough
        case DXGI_FORMAT_R16G16B16A16_FLOAT: // Intentional fallthrough
        case DXGI_FORMAT_R16G16B16A16_UINT:  // Intentional fallthrough
        case DXGI_FORMAT_R16G16B16A16_SINT:  // Intentional fallthrough
        case DXGI_FORMAT_R8G8B8A8_UNORM:     // Intentional fallthrough
        case DXGI_FORMAT_R8G8B8A8_UINT:      // Intentional fallthrough
        case DXGI_FORMAT_R8G8B8A8_SINT:      // Intentional fallthrough
        case DXGI_FORMAT_R32_FLOAT:          // Intentional fallthrough
        case DXGI_FORMAT_R32_UINT:           // Intentional fallthrough
        case DXGI_FORMAT_R32_SINT:           // Intentional fallthrough
        case DXGI_FORMAT_R16_FLOAT:          // Intentional fallthrough
        case DXGI_FORMAT_R16_UINT:           // Intentional fallthrough
        case DXGI_FORMAT_R16_SINT:           // Intentional fallthrough
        case DXGI_FORMAT_R8_UNORM:           // Intentional fallthrough
        case DXGI_FORMAT_R8_UINT:            // Intentional fallthrough
        case DXGI_FORMAT_R8_SINT:            return true;
        default:                             return false;
    }
}

inline bool gpu_texture::IsSRGBFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: // Intentional Fallthrough
        case DXGI_FORMAT_BC1_UNORM_SRGB:      // Intentional Fallthrough
        case DXGI_FORMAT_BC2_UNORM_SRGB:      // Intentional Fallthrough
        case DXGI_FORMAT_BC3_UNORM_SRGB:      // Intentional Fallthrough
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: // Intentional Fallthrough
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: // Intentional Fallthrough
        case DXGI_FORMAT_BC7_UNORM_SRGB:      return true;
        default:                              return false;
    }
}

inline bool gpu_texture::IsBGRFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:      // Intentional Fallthrough
        case DXGI_FORMAT_B8G8R8X8_UNORM:      // Intentional Fallthrough
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:   // Intentional Fallthrough
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: // Intentional Fallthrough
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:   // Intentional Fallthrough
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return true;
        default:                              return false;
    }
}

inline bool gpu_texture::IsDepthFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: // Intentional Fallthrough
        case DXGI_FORMAT_D32_FLOAT:            // Intentional Fallthrough
        case DXGI_FORMAT_D24_UNORM_S8_UINT:    // Intentional Fallthrough
        case DXGI_FORMAT_D16_UNORM:            return true;
        default:                               return false;
    }
}

inline DXGI_FORMAT gpu_texture::GetSRGBFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_BC1_UNORM:      return DXGI_FORMAT_BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM:      return DXGI_FORMAT_BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM:      return DXGI_FORMAT_BC3_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8X8_UNORM: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_BC7_UNORM:      return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:                         return Format; 
    }
}

inline DXGI_FORMAT gpu_texture::GetTypelessFormat(DXGI_FORMAT Format)
{
    DXGI_FORMAT Result = Format;

    switch (Format)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:   // Intentional Fallthrough
        case DXGI_FORMAT_R32G32B32A32_UINT:    // Intentional Fallthrough
        case DXGI_FORMAT_R32G32B32A32_SINT:    Result = DXGI_FORMAT_R32G32B32A32_TYPELESS; break;
        case DXGI_FORMAT_R32G32B32_FLOAT:      // Intentional Fallthrough
        case DXGI_FORMAT_R32G32B32_UINT:       // Intentional Fallthrough
        case DXGI_FORMAT_R32G32B32_SINT:       Result = DXGI_FORMAT_R32G32B32_TYPELESS;    break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:   // Intentional Fallthrough
        case DXGI_FORMAT_R16G16B16A16_UNORM:   // Intentional Fallthrough
        case DXGI_FORMAT_R16G16B16A16_UINT:    // Intentional Fallthrough
        case DXGI_FORMAT_R16G16B16A16_SNORM:   // Intentional Fallthrough
        case DXGI_FORMAT_R16G16B16A16_SINT:    Result = DXGI_FORMAT_R16G16B16A16_TYPELESS; break;
        case DXGI_FORMAT_R32G32_FLOAT:         // Intentional Fallthrough
        case DXGI_FORMAT_R32G32_UINT:          // Intentional Fallthrough
        case DXGI_FORMAT_R32G32_SINT:          Result = DXGI_FORMAT_R32G32_TYPELESS;       break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: Result = DXGI_FORMAT_R32G8X24_TYPELESS;     break;
        case DXGI_FORMAT_R10G10B10A2_UNORM:    // Intentional Fallthrough
        case DXGI_FORMAT_R10G10B10A2_UINT:     Result = DXGI_FORMAT_R10G10B10A2_TYPELESS;  break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:       // Intentional Fallthrough
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  // Intentional Fallthrough
        case DXGI_FORMAT_R8G8B8A8_UINT:        // Intentional Fallthrough
        case DXGI_FORMAT_R8G8B8A8_SNORM:       // Intentional Fallthrough
        case DXGI_FORMAT_R8G8B8A8_SINT:        Result = DXGI_FORMAT_R8G8B8A8_TYPELESS;     break;
        case DXGI_FORMAT_R16G16_FLOAT:         // Intentional Fallthrough
        case DXGI_FORMAT_R16G16_UNORM:         // Intentional Fallthrough
        case DXGI_FORMAT_R16G16_UINT:          // Intentional Fallthrough
        case DXGI_FORMAT_R16G16_SNORM:         // Intentional Fallthrough
        case DXGI_FORMAT_R16G16_SINT:          Result = DXGI_FORMAT_R16G16_TYPELESS;       break;
        case DXGI_FORMAT_D32_FLOAT:            // Intentional Fallthrough
        case DXGI_FORMAT_R32_FLOAT:            // Intentional Fallthrough
        case DXGI_FORMAT_R32_UINT:             // Intentional Fallthrough
        case DXGI_FORMAT_R32_SINT:             Result = DXGI_FORMAT_R32_TYPELESS;          break;
        case DXGI_FORMAT_R8G8_UNORM:           // Intentional Fallthrough
        case DXGI_FORMAT_R8G8_UINT:            // Intentional Fallthrough
        case DXGI_FORMAT_R8G8_SNORM:           // Intentional Fallthrough
        case DXGI_FORMAT_R8G8_SINT:            Result = DXGI_FORMAT_R8G8_TYPELESS;         break;
        case DXGI_FORMAT_R16_FLOAT:            // Intentional Fallthrough
        case DXGI_FORMAT_D16_UNORM:            // Intentional Fallthrough
        case DXGI_FORMAT_R16_UNORM:            // Intentional Fallthrough
        case DXGI_FORMAT_R16_UINT:             // Intentional Fallthrough
        case DXGI_FORMAT_R16_SNORM:            // Intentional Fallthrough
        case DXGI_FORMAT_R16_SINT:             Result = DXGI_FORMAT_R16_TYPELESS;          break;
        case DXGI_FORMAT_R8_UNORM:             // Intentional Fallthrough
        case DXGI_FORMAT_R8_UINT:              // Intentional Fallthrough
        case DXGI_FORMAT_R8_SNORM:             // Intentional Fallthrough
        case DXGI_FORMAT_R8_SINT:              Result = DXGI_FORMAT_R8_TYPELESS;           break;
        case DXGI_FORMAT_BC1_UNORM:            // Intentional Fallthrough
        case DXGI_FORMAT_BC1_UNORM_SRGB:       Result = DXGI_FORMAT_BC1_TYPELESS;          break;
        case DXGI_FORMAT_BC2_UNORM:            // Intentional Fallthrough
        case DXGI_FORMAT_BC2_UNORM_SRGB:       Result = DXGI_FORMAT_BC2_TYPELESS;          break;
        case DXGI_FORMAT_BC3_UNORM:            // Intentional Fallthrough
        case DXGI_FORMAT_BC3_UNORM_SRGB:       Result = DXGI_FORMAT_BC3_TYPELESS;          break;
        case DXGI_FORMAT_BC4_UNORM:            // Intentional Fallthrough
        case DXGI_FORMAT_BC4_SNORM:            Result = DXGI_FORMAT_BC4_TYPELESS;          break;
        case DXGI_FORMAT_BC5_UNORM:            // Intentional Fallthrough
        case DXGI_FORMAT_BC5_SNORM:            Result = DXGI_FORMAT_BC5_TYPELESS;          break;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:  Result = DXGI_FORMAT_B8G8R8A8_TYPELESS;     break;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:  Result = DXGI_FORMAT_B8G8R8X8_TYPELESS;     break;
        case DXGI_FORMAT_BC6H_UF16:            // Intentional Fallthrough
        case DXGI_FORMAT_BC6H_SF16:            Result = DXGI_FORMAT_BC6H_TYPELESS;         break;
        case DXGI_FORMAT_BC7_UNORM:            // Intentional Fallthrough  
        case DXGI_FORMAT_BC7_UNORM_SRGB:       Result = DXGI_FORMAT_BC7_TYPELESS;          break;
    }

    return Result;
}

inline DXGI_FORMAT gpu_texture::GetUAVCompatibleFormat(DXGI_FORMAT Format)
{
    DXGI_FORMAT Result = Format;

    switch (Format)
    {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:   // Intentional fallthrough
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: // Intentional fallthrough
        case DXGI_FORMAT_B8G8R8A8_UNORM:      // Intentional fallthrough
        case DXGI_FORMAT_B8G8R8X8_UNORM:      // Intentional fallthrough
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:   // Intentional fallthrough
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: // Intentional fallthrough
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:   // Intentional fallthrough
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: Result = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case DXGI_FORMAT_R32_TYPELESS:        // Intentional fallthrough
        case DXGI_FORMAT_D32_FLOAT:           Result = DXGI_FORMAT_R32_FLOAT; break;
    }

    return Result;
}
