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

void GpuTexture::releaseUnsafe(GpuDevice* tDevice)
{
    mResource.release();
    if (!mRTV.isNull())
    {
        tDevice->releaseDescriptors(mRTV, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }
    if (!mDSV.isNull())
    {
        tDevice->releaseDescriptors(mDSV, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }
    if (!mSRV.isNull())
    {
        tDevice->releaseDescriptors(mSRV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    if (!mUAV.isNull())
    {
        tDevice->releaseDescriptors(mUAV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

GpuTexture::GpuTexture(GpuDevice* tpDevice, GpuResource tResource) : mResource(tResource)
{
    createViews(tpDevice);
}

GpuTexture::GpuTexture(GpuDevice* tpDevice, const D3D12_RESOURCE_DESC &tDesc, std::optional<D3D12_CLEAR_VALUE> tClearValue)
: mResource(*tpDevice, tDesc, tClearValue)
{
    mResource.asHandle()->SetName(L"Texture2D");

    createViews(tpDevice);
    //tFrameCache->trackResource(mResource, D3D12_RESOURCE_STATE_COMMON);
}

void
GpuTexture::resize(GpuFrameCache* FrameCache, u32 Width, u32 Height)
{
    GpuDevice* pDevice = FrameCache->getDevice();
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
        AssertHr(pDevice->asHandle()->CreateCommittedResource(
            &HeapProperties, D3D12_HEAP_FLAG_NONE, &Desc,
            D3D12_RESOURCE_STATE_COMMON, ClearValuePtr, ComCast(&TempResource)));

        // TODO(enlynn): Resource is created in the COMMON state, do I need to transition it to a new state?

        mResource = GpuResource(*pDevice, TempResource, ClearValue);
        createViews(pDevice);
    }
}

void
GpuTexture::createViews(GpuDevice* tDevice)
{
    D3D12_RESOURCE_DESC Desc = mResource.getResourceDesc();

    // Create RTV
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 && checkRtvSupport())
    {
        mRTV = tDevice->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        tDevice->asHandle()->CreateRenderTargetView(mResource.asHandle(), nullptr, mRTV.getDescriptorHandle());
    }

    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 && checkDsvSupport())
    {
         mDSV = tDevice->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
         tDevice->asHandle()->CreateDepthStencilView(mResource.asHandle(), nullptr, mDSV.getDescriptorHandle());
    }

    if ((Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0 && checkSrvSupport())
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = Desc.Format;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        mSRV = tDevice->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        tDevice->asHandle()->CreateShaderResourceView(mResource.asHandle(), &srvDesc, mSRV.getDescriptorHandle());
    }

    // Create UAV for each mip (only supported for 1D and 2D textures).
    if ((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS ) != 0 && checkUavSupport() && Desc.DepthOrArraySize == 1 )
    {
         mUAV = tDevice->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Desc.MipLevels);
         for ( int i = 0; i < Desc.MipLevels; ++i )
         {
             D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = GetUAVDesc(&Desc, i);
             tDevice->asHandle()->CreateUnorderedAccessView(
                     mResource.asHandle(), nullptr, &UavDesc, mUAV.getDescriptorHandle(i));
         }
    }
}