#include "GpuResourceViews.h"
#include "GpuResource.h"
#include "GpuDevice.h"

GpuShaderResourceView::GpuShaderResourceView(std::shared_ptr<GpuDevice> tpDevice, 
    const GpuResource* tResource, D3D12_SHADER_RESOURCE_VIEW_DESC* tSrv)
: mDevice(tpDevice)
, mResource(tResource)
{
    mDescriptor = mDevice->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mDevice->createShaderResourceView(*this, tSrv);
}

void GpuShaderResourceView::release() {
    if (mDevice) {
        mDevice->releaseDescriptors(mDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        mResource   = nullptr;
        mDevice = nullptr;
        mDescriptor = {};
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuShaderResourceView::getDescriptorHandle() {
    return mDescriptor.getDescriptorHandle();
}

GpuUnorderedAccessView::GpuUnorderedAccessView(std::shared_ptr<GpuDevice> tpDevice, const GpuResource *tResource, 
    const GpuResource *tCounterResource, D3D12_UNORDERED_ACCESS_VIEW_DESC* tUav)
: mDevice(tpDevice)
, mResource(tResource)
, mCounterResource(tCounterResource)
{
    D3D12_RESOURCE_DESC desc = mResource->getResourceDesc();
    ASSERT((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0);

    mDescriptor = mDevice->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    mDevice->createUnorderedAccessView(*this, tUav);
}

void GpuUnorderedAccessView::release() {
    if (mDevice) {
        mDevice->releaseDescriptors(mDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        mResource        = nullptr;
        mCounterResource = nullptr;
        mDevice          = nullptr;
        mDescriptor      = {};
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuUnorderedAccessView::getDescriptorHandle() {
    return mDescriptor.getDescriptorHandle();
}
