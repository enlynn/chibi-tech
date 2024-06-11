#include "GpuTexture.h"
#include "GpuState.h"
#include "GpuUtils.h"
#include "GpuResource.h"

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

void GpuTexture::releaseUnsafe(GpuFrameCache* FrameCache)
{
    mResource.release();
    if (!mRTV.isNull())
    {
        FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].releaseDescriptors(mRTV);
    }
    if (!mDSV.isNull())
    {
        FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].releaseDescriptors(mDSV);
    }
    if (!mSRV.isNull())
    {
        FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].releaseDescriptors(mSRV);
    }
    if (!mUAV.isNull())
    {
        FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].releaseDescriptors(mUAV);
    }
}

GpuTexture::GpuTexture(GpuFrameCache* FrameCache, GpuResource Resource) : mResource(Resource)
{
    createViews(FrameCache);
}

void
GpuTexture::resize(GpuFrameCache* FrameCache, u32 Width, u32 Height)
{
    GpuDevice*         Device = FrameCache->getDevice();
    D3D12_RESOURCE_DESC Desc   = mResource.getResourceDesc();

    if (Desc.Width != Width || Desc.Height == Height)
    {
        Desc.Width  = Width;
        Desc.Height = Height;
        Desc.MipLevels = Desc.SampleDesc.Count > 1 ? 1 : 0;

        std::optional<D3D12_CLEAR_VALUE> ClearValue = mResource.getClearValue();
        D3D12_CLEAR_VALUE* ClearValuePtr = ClearValue.has_value() ? &ClearValue.value() : nullptr;

        // Garbage collect the current Resource
        FrameCache->addStaleResource(mResource);

        ID3D12Resource* TempResource = nullptr;

        D3D12_HEAP_PROPERTIES HeapProperties = getHeapProperties();
        AssertHr(Device->asHandle()->CreateCommittedResource(
            &HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc,
            D3D12_RESOURCE_STATE_COMMON, ClearValuePtr, ComCast(&TempResource)));

        // TODO(enlynn): Resource is created in the COMMON state, do I need to transition it to a new state?

        mResource = GpuResource(*Device, TempResource, ClearValue);
        createViews(FrameCache);
    }
}

void
GpuTexture::createViews(GpuFrameCache* FrameCache)
{
    GpuDevice*         Device = FrameCache->getDevice();
    D3D12_RESOURCE_DESC Desc   = mResource.getResourceDesc();

    // Create RTV
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && checkRtvSupport())
    {
         mRTV = FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_RTV].allocate();
        Device->asHandle()->CreateRenderTargetView(mResource.asHandle(), nullptr, mRTV.getDescriptorHandle());
    }

    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && checkDsvSupport())
    {
         mDSV = FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_DSV].allocate();
        Device->asHandle()->CreateDepthStencilView(mResource.asHandle(), nullptr, mDSV.getDescriptorHandle());
    }

    if ((Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 && checkSrvSupport())
    {
         mSRV = FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].allocate();
        Device->asHandle()->CreateShaderResourceView(mResource.asHandle(), nullptr, mSRV.getDescriptorHandle());
    }

    // Create UAV for each mip (only supported for 1D and 2D textures).
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ) != 0 && checkUavSupport() && Desc.DepthOrArraySize == 1 )
    {
         mUAV = FrameCache->mGlobal->mStaticDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].allocate(Desc.MipLevels);
         for ( int i = 0; i < Desc.MipLevels; ++i )
         {
             D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = GetUAVDesc(&Desc, i);
             Device->asHandle()->CreateUnorderedAccessView(
                     mResource.asHandle(), nullptr, &UavDesc, mUAV.getDescriptorHandle(i));
         }
    }
}