#include "gpu_texture.h"
#include "GpuState.h"
#include "gpu_utils.h"
#include "gpu_resource.h"

static D3D12_UNORDERED_ACCESS_VIEW_DESC
GetUAVDesc(D3D12_RESOURCE_DESC* ResDesc, UINT MipSlice, UINT ArraySlice = 0, UINT PlaneSlice = 0)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
    UavDesc.Format                           = ResDesc->Format;

    switch (ResDesc->Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        {
            if (ResDesc->DepthOrArraySize > 1)
            {
                UavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                UavDesc.Texture1DArray.ArraySize       = ResDesc->DepthOrArraySize - ArraySlice;
                UavDesc.Texture1DArray.FirstArraySlice = ArraySlice;
                UavDesc.Texture1DArray.MipSlice        = MipSlice;
            }
            else
            {
                UavDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
                UavDesc.Texture1D.MipSlice = MipSlice;
            }
        } break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        {
            if ( ResDesc->DepthOrArraySize > 1 )
            {
                UavDesc.ViewDimension                  = (ResDesc->SampleDesc.Count > 1) ? D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY : D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                UavDesc.Texture2DArray.ArraySize       = ResDesc->DepthOrArraySize - ArraySlice;
                UavDesc.Texture2DArray.FirstArraySlice = ArraySlice;
                UavDesc.Texture2DArray.PlaneSlice      = PlaneSlice;
                UavDesc.Texture2DArray.MipSlice        = MipSlice;
            }
            else
            {
                UavDesc.ViewDimension        = (ResDesc->SampleDesc.Count > 1) ? D3D12_UAV_DIMENSION_TEXTURE2DMS : D3D12_UAV_DIMENSION_TEXTURE2D;
                UavDesc.Texture2D.PlaneSlice = PlaneSlice;
                UavDesc.Texture2D.MipSlice   = MipSlice;
            }

            if (ResDesc->SampleDesc.Count > 0)
            {}
        } break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        {
            UavDesc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
            UavDesc.Texture3D.WSize       = ResDesc->DepthOrArraySize - ArraySlice;
            UavDesc.Texture3D.FirstWSlice = ArraySlice;
            UavDesc.Texture3D.MipSlice    = MipSlice;
        } break;

        default: ASSERT(false && "Invalid Resource dimension.");
    }

    return UavDesc;
}

void gpu_texture::ReleaseUnsafe(gpu_frame_cache* FrameCache)
{
    mResource.Release();
    if (!mRTV.IsNull())
    {
        FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].ReleaseDescriptors(mRTV);
    }
    if (!mDSV.IsNull())
    {
        FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].ReleaseDescriptors(mDSV);
    }
    if (!mSRV.IsNull())
    {
        FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].ReleaseDescriptors(mSRV);
    }
    if (!mUAV.IsNull())
    {
        FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].ReleaseDescriptors(mUAV);
    }
}

gpu_texture::gpu_texture(gpu_frame_cache* FrameCache, gpu_resource Resource) : mResource(Resource)
{
    CreateViews(FrameCache);
}

void
gpu_texture::Resize(gpu_frame_cache* FrameCache, u32 Width, u32 Height)
{
    gpu_device*         Device = FrameCache->GetDevice();
    D3D12_RESOURCE_DESC Desc   = mResource.GetResourceDesc();

    if (Desc.Width != Width || Desc.Height == Height)
    {
        Desc.Width  = Width;
        Desc.Height = Height;
        Desc.MipLevels = Desc.SampleDesc.Count > 1 ? 1 : 0;

        std::optional<D3D12_CLEAR_VALUE> ClearValue = mResource.GetClearValue();
        D3D12_CLEAR_VALUE* ClearValuePtr = ClearValue.has_value() ? &ClearValue.value() : nullptr;

        // Garbage collect the current Resource
        FrameCache->AddStaleResource(mResource);

        ID3D12Resource* TempResource = nullptr;

        D3D12_HEAP_PROPERTIES HeapProperties = GetHeapProperties();
        AssertHr(Device->AsHandle()->CreateCommittedResource(
            &HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc,
            D3D12_RESOURCE_STATE_COMMON, ClearValuePtr, ComCast(&TempResource)));

        // TODO(enlynn): Resource is created in the COMMON state, do I need to transition it to a new state?

        mResource = gpu_resource(*Device, TempResource, ClearValue);
        CreateViews(FrameCache);
    }
}

void
gpu_texture::CreateViews(gpu_frame_cache* FrameCache)
{
    gpu_device*         Device = FrameCache->GetDevice();
    D3D12_RESOURCE_DESC Desc   = mResource.GetResourceDesc();

    // Create RTV
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && CheckRTVSupport())
    {
         mRTV = FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].Allocate();
         Device->AsHandle()->CreateRenderTargetView( mResource.AsHandle(), nullptr, mRTV.GetDescriptorHandle());
    }

    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && CheckDSVSupport())
    {
         mDSV = FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].Allocate();
         Device->AsHandle()->CreateDepthStencilView(mResource.AsHandle(), nullptr, mDSV.GetDescriptorHandle());
    }

    if ((Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 && CheckSRVSupport())
    {
         mSRV = FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate();
         Device->AsHandle()->CreateShaderResourceView(mResource.AsHandle(), nullptr, mSRV.GetDescriptorHandle());
    }

    // Create UAV for each mip (only supported for 1D and 2D textures).
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ) != 0 && CheckUAVSupport() && Desc.DepthOrArraySize == 1 )
    {
         mUAV = FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Allocate(Desc.MipLevels);
         for ( int i = 0; i < Desc.MipLevels; ++i )
         {
             D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = GetUAVDesc(&Desc, i);
             Device->AsHandle()->CreateUnorderedAccessView(
                 mResource.AsHandle(), nullptr, &UavDesc, mUAV.GetDescriptorHandle(i));
         }
    }
}