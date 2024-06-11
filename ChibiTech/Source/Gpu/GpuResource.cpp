#include "stdafx.h"
#include "GpuResource.h"
#include "GpuDevice.h"
#include "GpuUtils.h"

#include <Platform/Console.h>

GpuResource::GpuResource(const GpuDevice& Device, const D3D12_RESOURCE_DESC& Desc, std::optional <D3D12_CLEAR_VALUE> ClearValue)
	: mClearValue(ClearValue)
{
	D3D12_HEAP_PROPERTIES Properties = getHeapProperties();
	D3D12_CLEAR_VALUE*    Value      = mClearValue.has_value() ? &mClearValue.value() : nullptr;

    ID3D12Device2* rawDevice = Device.asHandle();
    ASSERT(rawDevice);

    HRESULT hr = rawDevice->CreateCommittedResource(&Properties, D3D12_HEAP_FLAG_NONE, &Desc,
                                                    D3D12_RESOURCE_STATE_COMMON, Value, ComCast(&mHandle));
    if (FAILED(hr)) {
        ct::console::fatal("Failed to create GpuResource with error: %x", hr);
        AssertHr(hr);
    }

	// TODO(enlynn): global Resource management? -> State: D3D12_RESOURCE_STATE_COMMON

    checkFeatureSupport(Device);
}

GpuResource::GpuResource(const GpuDevice& Device, ID3D12Resource* Resource, std::optional <D3D12_CLEAR_VALUE> ClearValue)
	: mClearValue(ClearValue)
	, mHandle(Resource)
{
    checkFeatureSupport(Device);
}

void 
GpuResource::release()
{
	if (isValid())
	{
		ComSafeRelease(mHandle);
		mClearValue = std::nullopt;
		mFormatSupport = {};
	}
}

void
GpuResource::checkFeatureSupport(const GpuDevice& Device)
{
	D3D12_RESOURCE_DESC ResourceDesc = mHandle->GetDesc();
	mFormatSupport.Format = ResourceDesc.Format;
	AssertHr(Device.asHandle()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &mFormatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)));
}

bool
GpuResource::checkFormatSupport(D3D12_FORMAT_SUPPORT1 FormatSupport)
{
	return (mFormatSupport.Support1 & FormatSupport) != 0;
}

bool
GpuResource::checkFormatSupport(D3D12_FORMAT_SUPPORT2 FormatSupport)
{
	return (mFormatSupport.Support2 & FormatSupport) != 0;
}

D3D12_RESOURCE_DESC 
GpuResource::getResourceDesc() const
{
	D3D12_RESOURCE_DESC Result = {};
	if (isValid())
	{
		Result = mHandle->GetDesc();
	}
	return Result;
}