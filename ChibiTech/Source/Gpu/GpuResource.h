#pragma once

#include <types.h>
#include "D3D12Common.h"

#include <optional>

class GpuDevice;

class GpuResource
{
public:
	GpuResource() = default;
	// Usually external code will not directly initialize a GpuResource. Usually it will be a Swapchain or Storage Buffer
	GpuResource(const GpuDevice& Device, const D3D12_RESOURCE_DESC& Desc, std::optional <D3D12_CLEAR_VALUE> ClearValue = std::nullopt);
	// Assumes ownership of the Resource
	GpuResource(const GpuDevice& Device, ID3D12Resource* Resource, std::optional <D3D12_CLEAR_VALUE> ClearValue = std::nullopt);

	~GpuResource() {} // For a GpuResource, we need to explicitly clear the value to prevent erroneous Resource frees
	
	void                                    release();

	bool                                    checkFormatSupport(D3D12_FORMAT_SUPPORT1 FormatSupport);
	bool                                    checkFormatSupport(D3D12_FORMAT_SUPPORT2 FormatSupport);
	// Check the format support and populate the mFormatSupport structure.
	void                                    checkFeatureSupport(const GpuDevice& Device);

	D3D12_RESOURCE_DESC                     getResourceDesc() const;

    D3D12_GPU_VIRTUAL_ADDRESS               getGpuAddress() const { return mHandle->GetGPUVirtualAddress(); }

	inline ID3D12Resource*                  asHandle()      const { return mHandle;            }
	inline bool                             isValid()       const { return mHandle != nullptr; }
	inline std::optional<D3D12_CLEAR_VALUE> getClearValue() const { return mClearValue;        }

private:
	ID3D12Resource*                   mHandle        = nullptr;
	D3D12_FEATURE_DATA_FORMAT_SUPPORT mFormatSupport = {};
	std::optional<D3D12_CLEAR_VALUE>  mClearValue    = {};
};