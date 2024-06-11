#pragma once

#include <types.h>
#include "D3D12Common.h"

#include <optional>

class gpu_device;

class gpu_resource
{
public:
	gpu_resource() = default;
	// Usually external code will not directly initialize a gpu_resource. Usually it will be a Swapchain or Storage Buffer
	gpu_resource(const gpu_device& Device, const D3D12_RESOURCE_DESC& Desc, std::optional <D3D12_CLEAR_VALUE> ClearValue = std::nullopt);
	// Assumes ownership of the Resource
	gpu_resource(const gpu_device& Device, ID3D12Resource* Resource, std::optional <D3D12_CLEAR_VALUE> ClearValue = std::nullopt);

	~gpu_resource() {} // For a gpu_resource, we need to explicitly clear the value to prevent erroneous Resource frees
	
	void                                    Release();

	bool                                    CheckFormatSupport(D3D12_FORMAT_SUPPORT1 FormatSupport);
	bool                                    CheckFormatSupport(D3D12_FORMAT_SUPPORT2 FormatSupport);
	// Check the format support and populate the mFormatSupport structure.
	void                                    CheckFeatureSupport(const gpu_device& Device);

	D3D12_RESOURCE_DESC                     GetResourceDesc() const;

    D3D12_GPU_VIRTUAL_ADDRESS               GetGPUAddress() const { return mHandle->GetGPUVirtualAddress(); }

	inline ID3D12Resource*                  AsHandle()      const { return mHandle;            }
	inline bool                             IsValid()       const { return mHandle != nullptr; }
	inline std::optional<D3D12_CLEAR_VALUE> GetClearValue() const { return mClearValue;        }

private:
	ID3D12Resource*                   mHandle        = nullptr;
	D3D12_FEATURE_DATA_FORMAT_SUPPORT mFormatSupport = {};
	std::optional<D3D12_CLEAR_VALUE>  mClearValue    = {};
};

#if 0
enum class gpu_resource_view_type
{
	srv, // Shader Resource View
	uav, // Unordered Access View
};

struct gpu_resource_view
{
	// Create a Shader Resource View
	gpu_resource_view(gpu_resource& Resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& SRV);
	// Create a Unordered Access Resource View
	gpu_resource_view(gpu_resource& Resource, gpu_resource& CounterResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& SRV);

	~gpu_resource_view() {} // For a gpu_resource_view, we need to explicitly clear to prevent erroneous Resource frees
	void Deinit();



	gpu_resource_view_type mType;
	gpu_resource&          mBorrowedResource;
	gpu_resource&          mBorrowedUAVCounterResource; // Optional, for UAV
};
#endif
