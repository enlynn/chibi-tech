#include "stdafx.h"
#include "gpu_resource.h"
#include "gpu_device.h"
#include "gpu_utils.h"

#include <Platform/Console.h>

gpu_resource::gpu_resource(const gpu_device& Device, const D3D12_RESOURCE_DESC& Desc, std::optional <D3D12_CLEAR_VALUE> ClearValue)
	: mClearValue(ClearValue)
{
	D3D12_HEAP_PROPERTIES Properties = GetHeapProperties();
	D3D12_CLEAR_VALUE*    Value      = mClearValue.has_value() ? &mClearValue.value() : nullptr;

    ID3D12Device2* rawDevice = Device.AsHandle();
    ASSERT(rawDevice);

    HRESULT hr = rawDevice->CreateCommittedResource(&Properties, D3D12_HEAP_FLAG_NONE, &Desc,
                                                    D3D12_RESOURCE_STATE_COMMON, Value, ComCast(&mHandle));
    if (FAILED(hr)) {
        ct::console::fatal("Failed to create gpu_resource with error: %x", hr);
        AssertHr(hr);
    }

	// TODO(enlynn): global Resource management? -> State: D3D12_RESOURCE_STATE_COMMON

	CheckFeatureSupport(Device);
}

gpu_resource::gpu_resource(const gpu_device& Device, ID3D12Resource* Resource, std::optional <D3D12_CLEAR_VALUE> ClearValue)
	: mClearValue(ClearValue)
	, mHandle(Resource)
{
	CheckFeatureSupport(Device);
}

void 
gpu_resource::Release()
{
	if (IsValid())
	{
		ComSafeRelease(mHandle);
		mClearValue = std::nullopt;
		mFormatSupport = {};
	}
}

void
gpu_resource::CheckFeatureSupport(const gpu_device& Device)
{
	D3D12_RESOURCE_DESC ResourceDesc = mHandle->GetDesc();
	mFormatSupport.Format = ResourceDesc.Format;
	AssertHr(Device.AsHandle()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &mFormatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
}

bool
gpu_resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
{
	return (mFormatSupport.Support1 & formatSupport) != 0;
}

bool
gpu_resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport)
{
	return (mFormatSupport.Support2 & formatSupport) != 0;
}

D3D12_RESOURCE_DESC 
gpu_resource::GetResourceDesc() const
{
	D3D12_RESOURCE_DESC Result = {};
	if (IsValid())
	{
		Result = mHandle->GetDesc();
	}
	return Result;
}