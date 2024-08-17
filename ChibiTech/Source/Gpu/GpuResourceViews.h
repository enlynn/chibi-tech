#pragma once

#include <memory>

#include "D3D12Common.h"
#include "GpuDescriptorAllocator.h"
#include "GpuResource.h"

class GpuDevice;

class GpuShaderResourceView {
public:
    GpuShaderResourceView(std::shared_ptr<GpuDevice> tpDevice, const GpuResource* tResource, D3D12_SHADER_RESOURCE_VIEW_DESC* tSrv = nullptr);
    ~GpuShaderResourceView() { release(); }

    void release();

    D3D12_CPU_DESCRIPTOR_HANDLE getDescriptorHandle();
    const GpuResource*          getResource() const { return mResource; }

private:
    // TODO(enlynn): don't store a raw pointer like this.
    std::shared_ptr<GpuDevice> mDevice{ nullptr };
    const GpuResource*         mResource{nullptr};
    CpuDescriptor              mDescriptor{};
};

class GpuUnorderedAccessView {
public:
    GpuUnorderedAccessView(std::shared_ptr<GpuDevice> tpDevice, const GpuResource* tResource, const GpuResource* tCounterResource, D3D12_UNORDERED_ACCESS_VIEW_DESC* tSrv = nullptr);
    ~GpuUnorderedAccessView() { release(); }

    void release();

    D3D12_CPU_DESCRIPTOR_HANDLE getDescriptorHandle();

    const GpuResource*          getResource()        const { return mResource;        }
    const GpuResource*          getCounterResource() const { return mCounterResource; }

private:
    // TODO(enlynn): don't store a raw pointer like this.
    std::shared_ptr<GpuDevice> mDevice{nullptr};
    const GpuResource*         mResource{nullptr};
    const GpuResource*         mCounterResource{nullptr};
    CpuDescriptor              mDescriptor{};
};