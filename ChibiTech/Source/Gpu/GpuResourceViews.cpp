#include "GpuResourceViews.h"
#include "GpuResource.h"
#include "GpuState.h"

GpuShaderResourceView::GpuShaderResourceView(GpuState* tGpuContext, const GpuResource *tResource,
                                       D3D12_SHADER_RESOURCE_VIEW_DESC *tSrv)
: mGpuContext(tGpuContext)
, mResource(tResource)
{
    mDescriptor = tGpuContext->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    tGpuContext->mDevice->createShaderResourceView(*this, tSrv);
}

void GpuShaderResourceView::release() {
    if (mGpuContext) {
        mGpuContext->releaseDescriptors(mDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        mResource   = nullptr;
        mGpuContext = nullptr;
        mDescriptor = {};
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuShaderResourceView::getDescriptorHandle() {
    return mDescriptor.getDescriptorHandle();
}

GpuUnorderedAccessView::GpuUnorderedAccessView(GpuState* tGpuContext, const GpuResource *tResource, const GpuResource *tCounterResource,
                                               D3D12_UNORDERED_ACCESS_VIEW_DESC* tUav)
: mGpuContext(tGpuContext)
, mResource(tResource)
, mCounterResource(tCounterResource)
{
    D3D12_RESOURCE_DESC desc = mResource->getResourceDesc();
    ASSERT((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);

    mDescriptor = tGpuContext->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    tGpuContext->mDevice->createUnorderedAccessView(*this, tUav);
}

void GpuUnorderedAccessView::release() {
    if (mGpuContext) {
        mGpuContext->releaseDescriptors(mDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        mResource        = nullptr;
        mCounterResource = nullptr;
        mGpuContext      = nullptr;
        mDescriptor      = {};
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuUnorderedAccessView::getDescriptorHandle() {
    return mDescriptor.getDescriptorHandle();
}
