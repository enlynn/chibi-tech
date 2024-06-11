#pragma once

#include <types.h>

#include "D3D12Common.h"
#include "GpuResource.h"

struct commited_resource_info
{
    D3D12_HEAP_TYPE          HeapType      = D3D12_HEAP_TYPE_DEFAULT;
    u64                      Size          = 0;
    u64                      Alignment     = 0;
    D3D12_RESOURCE_FLAGS     ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    D3D12_HEAP_FLAGS         HeapFlags     = D3D12_HEAP_FLAG_NONE;
    D3D12_RESOURCE_STATES    InitialState  = D3D12_RESOURCE_STATE_COMMON;
    const D3D12_CLEAR_VALUE* ClearValue    = nullptr;
};

class GpuDevice
{
public:
	GpuDevice() = default;
	void init();

	void deinit();
	~GpuDevice() { deinit(); }

	void reportLiveObjects();

	// Cast to the internal ID3D12Device handle
	constexpr ID3D12Device2* asHandle()  const { return mDevice;  }
	constexpr IDXGIAdapter1* asAdapter() const { return mAdapter; }

    DXGI_SAMPLE_DESC getMultisampleQualityLevels(
                DXGI_FORMAT                           Format,
                u32                                   NumSamples = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT,
                D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS Flags      = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE);

	ID3D12DescriptorHeap* createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 Count, bool IsShaderVisible);

    GpuResource          createCommittedResource(commited_resource_info& Info);

private:
	ID3D12Device2* mDevice                = nullptr;
	IDXGIAdapter1* mAdapter               = nullptr;
	u32            mSupportedFeatureLevel = 0;

	void enableDebugDevice();
	void selectAdapter();

	// TODO: requested and enabled features.
};
