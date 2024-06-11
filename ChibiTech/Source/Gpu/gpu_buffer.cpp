#include "GpuBuffer.h"

#include "gpu_utils.h"
#include "gpu_device.h"
#include "GpuState.h"

gpu_resource
GpuBuffer::CopyBuffer(gpu_frame_cache* FrameCache, void* BufferData, u64 BufferSize, D3D12_RESOURCE_FLAGS Flags, D3D12_RESOURCE_STATES InitialBufferState)
{
    gpu_device*       Device      = FrameCache->GetDevice();
    gpu_command_list* CommandList = FrameCache->GetCopyCommandList();

	gpu_resource Result = {};
	if (BufferSize == 0)
	{
		// This will result in a NULL Resource (which may be desired to define a default null Resource).
	}
	else
	{
        commited_resource_info ResourceInfo = {};
        ResourceInfo.Size                   = BufferSize;
        Result = Device->CreateCommittedResource(ResourceInfo);

		// TODO: Track Resource state.

		if (BufferData != nullptr)
		{
            commited_resource_info ResourceInfo = {};
            ResourceInfo.HeapType               = D3D12_HEAP_TYPE_UPLOAD;
            ResourceInfo.Size                   = BufferSize;
            ResourceInfo.InitialState           = D3D12_RESOURCE_STATE_GENERIC_READ;
            gpu_resource UploadResource         = Device->CreateCommittedResource(ResourceInfo);

			D3D12_SUBRESOURCE_DATA SubresourceData = {};
			SubresourceData.pData = BufferData;
			SubresourceData.RowPitch = (LONG_PTR)BufferSize;
			SubresourceData.SlicePitch = SubresourceData.RowPitch;

			{
				gpu_transition_barrier BufferCopyDstBarrier = {};
				BufferCopyDstBarrier.BeforeState = D3D12_RESOURCE_STATE_COMMON;
				BufferCopyDstBarrier.AfterState = D3D12_RESOURCE_STATE_COPY_DEST;
                CommandList->TransitionBarrier(Result, BufferCopyDstBarrier);
			}

			CommandList->UpdateSubresources<1>(
				Result, UploadResource, 0,
				0, 1, farray(&SubresourceData, 1)
			);

			// Add references to resources so they stay in scope until the command list is reset.
            FrameCache->AddStaleResource(UploadResource);
		}

		//TrackResource(result);
	}

	{ // Transition the buffer back to be used
		gpu_transition_barrier BufferBarrier = {};
		BufferBarrier.BeforeState = D3D12_RESOURCE_STATE_COPY_DEST;
		BufferBarrier.AfterState = InitialBufferState;
		CommandList->TransitionBarrier(Result, BufferBarrier);
	}

	return Result;
}

GpuBuffer
GpuBuffer::CreateByteAdressBuffer(gpu_frame_cache* FrameCache, gpu_byte_address_buffer_info& Info)
{
    u64 FrameSize  = Info.mCount * Info.mStride;
	u64 BufferSize = Info.mBufferFrames * FrameSize;

	// Not sure if this should be D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER or D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	gpu_resource BufferResource = CopyBuffer(
        FrameCache, Info.mData, BufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );

	GpuBuffer Result = {};
	Result.mType         = gpu_buffer_type::byte_address;
	Result.mResource     = BufferResource;
    Result.mCount        = Info.mCount;
    Result.mStride       = Info.mStride;
    Result.mBufferFrames = Info.mBufferFrames;

	// Shader Resource View?
    if (Info.mCreateResourceView)
    {
        // TODO(enlynn):
    }

    // Track the Resource state
    FrameCache->TrackResource(BufferResource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	return Result;
}

GpuBuffer
GpuBuffer::CreateIndexBuffer(gpu_frame_cache* FrameCache, gpu_index_buffer_info& Info)
{
    u64 Stride = Info.mIsU16 ? sizeof(u16)
                             : sizeof(u32);

    DXGI_FORMAT IndexFormat = (Info.mIsU16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	u64          BufferSize = Info.mIndexCount * Stride;

	GpuBuffer Result = {};
	Result.mType     = gpu_buffer_type::index;
    Result.mResource = CopyBuffer(FrameCache, Info.mIndices, BufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	Result._views.mIndexView.BufferLocation = Result.mResource.AsHandle()->GetGPUVirtualAddress();
    Result._views.mIndexView.Format         = IndexFormat;
    Result._views.mIndexView.SizeInBytes    = (UINT)(BufferSize);

	Result.mStride = (u8)Stride;
	Result.mCount  = Info.mIndexCount;

    // Track the Resource state
    FrameCache->TrackResource(Result.mResource, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	return Result;
}

GpuBuffer GpuBuffer::CreateStructuredBuffer(gpu_frame_cache* FrameCache, gpu_structured_buffer_info& Info)
{
    D3D12_RESOURCE_FLAGS  Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    u64 BufferSize                           = Info.mFrames * Info.mCount * Info.mStride;

    //CopyBuffer(FrameCache, nullptr, u64 BufferSize, Flags, InitialBufferState)

    commited_resource_info ResourceInfo = {};
    ResourceInfo.Size                   = BufferSize;
    ResourceInfo.HeapType               = D3D12_HEAP_TYPE_UPLOAD;
    ResourceInfo.InitialState           = D3D12_RESOURCE_STATE_GENERIC_READ; // ...is this right?
    //ResourceInfo.ResourceFlags          = Flags;
    gpu_resource Resource = FrameCache->GetDevice()->CreateCommittedResource(ResourceInfo);
    Resource.AsHandle()->SetName(L"PER MESH DATA");

    GpuBuffer Result    = {};
    Result.mType         = gpu_buffer_type::structured;
    Result.mResource     = Resource;
    Result.mCpuVisible   = true;
    Result.mCount        = (u32)Info.mCount;
    Result.mStride       = (u32)Info.mStride;
    Result.mBufferFrames = Info.mFrames;

    // Track the Resource state
    FrameCache->TrackResource(Resource, D3D12_RESOURCE_STATE_GENERIC_READ);

    return Result;
}

void GpuBuffer::Map(u64 Frame)
{ // NOTE(enlynn): probably want to map only a small range, but being lazy for now...
    mResource.AsHandle()->Map(0, nullptr, (void**)&mMappedData);
    mMappedFrame = (Frame % mBufferFrames);
    mFrameData   = (u8*)mMappedData + mMappedFrame * (mCount * mStride);
}

void GpuBuffer::Unmap()
{
    mResource.AsHandle()->Unmap(0, nullptr);
    mFrameData  = nullptr;
    mMappedData = nullptr;
    mMappedFrame = 0;
}