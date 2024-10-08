#pragma once

#include <memory>
#include <array>

#include <Types.h>

#include "D3D12Common.h"
#include "GpuResource.h"
#include "GpuDescriptorAllocator.h"

class GpuShaderResourceView;
class GpuUnorderedAccessView;

struct CommitedResourceInfo
{
    D3D12_HEAP_TYPE          HeapType      = D3D12_HEAP_TYPE_DEFAULT;
    u64                      Size          = 0;
    u64                      Alignment     = 0;
    D3D12_RESOURCE_FLAGS     ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_FLAGS         HeapFlags     = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES    InitialState  = D3D12_RESOURCE_STATE_COMMON;
    const D3D12_CLEAR_VALUE* ClearValue    = nullptr;
};

struct PlacedResourceInfo {
    ID3D12Heap*                mHeap{};
    const D3D12_RESOURCE_DESC* mDesc{};
    D3D12_RESOURCE_STATES      mInitialState{};
    UINT64                     mHeapOffset{0};
    const D3D12_CLEAR_VALUE*   mOptimizedClearValue{nullptr};
};

struct GpuDeviceInfo
{
    bool mEnableMSAA{ false };
    bool mPreferHDR{ false };              // HDR refers to target output (10bit color channel, 2bit alpha)
    bool mEnableHDRRenderTargets{ false }; // HDRRenderTargets refers to using 16bit color channels, will be automatically 
};

class GpuDevice
{
public:
	GpuDevice(GpuDeviceInfo&& tInfo);

	void deinit();
	~GpuDevice() { deinit(); }

	void reportLiveObjects();

	// Cast to the internal ID3D12Device handle
	constexpr ID3D12Device2* asHandle()  const { return mDevice;  }
	constexpr IDXGIAdapter1* asAdapter() const { return mAdapter; }

    DXGI_FORMAT getDisplayFormat(); // Will get the display format (10bit if HDR, 8bit if SDR)
    DXGI_FORMAT getTargetFormat();  // Will get the generic render target format (16bit if HDR Render Targets, 8bit otherwise)
    DXGI_FORMAT getDepthFormat();

    DXGI_SAMPLE_DESC getMultisampleQualityLevels(
                DXGI_FORMAT                           Format,
                u32                                   NumSamples = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT,
                D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS Flags      = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE);

	ID3D12DescriptorHeap* createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 Count, bool IsShaderVisible);

    GpuResource createCommittedResource(CommitedResourceInfo& Info);
    GpuResource createPlacedResource(const PlacedResourceInfo& tInfo);
    void        createShaderResourceView(GpuShaderResourceView& tSrv, const D3D12_SHADER_RESOURCE_VIEW_DESC* tDesc);
    void        createUnorderedAccessView(GpuUnorderedAccessView& tSrv, const D3D12_UNORDERED_ACCESS_VIEW_DESC* tDesc);

    // cpu descriptors
    CpuDescriptor allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE tDescriptorType, u32 tNumDescriptors = 1);
    void releaseDescriptors(CpuDescriptor tDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE tDescriptorType);

private:
    GpuDeviceInfo  mInfo{};
	ID3D12Device2* mDevice                = nullptr;
	IDXGIAdapter1* mAdapter               = nullptr;
    u32            mSupportedFeatureLevel{ 0 };

    //todo: hdr isn't technically supported
    bool           mHDREnabled{ false };

    // cpu visible descriptor pools
    std::array<CpuDescriptorAllocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mStaticDescriptors;  // Global descriptors, long lived

	void enableDebugDevice();
	void selectAdapter();
};

using GpuDeviceSPtr = std::shared_ptr<GpuDevice>;