#include "GpuBuffer.h"

#include "GpuUtils.h"
#include "GpuDevice.h"
#include "GpuState.h"

GpuResource
GpuBuffer::copyBuffer(struct GpuFrameCache *FrameCache, void* BufferData, u64 BufferSize, D3D12_RESOURCE_FLAGS Flags, D3D12_RESOURCE_STATES InitialBufferState)
{
    GpuDevice*       Device      = FrameCache->getDevice();
    GpuCommandList* CommandList = FrameCache->getCopyCommandList();

	GpuResource Result = {};
	if (BufferSize == 0)
	{
		// This will result in a NULL Resource (which may be desired to define a default null Resource).
	}
	else
	{
        commited_resource_info ResourceInfo = {};
        ResourceInfo.Size                   = BufferSize;
        Result = Device->createCommittedResource(ResourceInfo);

		// TODO: Track Resource state.

		if (BufferData != nullptr)
		{
            commited_resource_info ResourceInfo = {};
            ResourceInfo.HeapType               = D3D12_HEAP_TYPE_UPLOAD;
            ResourceInfo.Size                   = BufferSize;
            ResourceInfo.InitialState           = D3D12_RESOURCE_STATE_GENERIC_READ;
            GpuResource UploadResource         = Device->createCommittedResource(ResourceInfo);

			D3D12_SUBRESOURCE_DATA SubresourceData = {};
			SubresourceData.pData = BufferData;
			SubresourceData.RowPitch = (LONG_PTR)BufferSize;
			SubresourceData.SlicePitch = SubresourceData.RowPitch;

			{
				GpuTransitionBarrier BufferCopyDstBarrier = {};
				BufferCopyDstBarrier.mBeforeState = D3D12_RESOURCE_STATE_COMMON;
				BufferCopyDstBarrier.mAfterState = D3D12_RESOURCE_STATE_COPY_DEST;
                CommandList->transitionBarrier(Result, BufferCopyDstBarrier);
			}

            CommandList->updateSubresources<1>(
                    Result, UploadResource, 0,
                    0, 1, farray(&SubresourceData, 1)
            );

			// Add references to resources so they stay in scope until the command list is reset.
            FrameCache->addStaleResource(UploadResource);
		}

		//trackResource(result);
	}

	{ // Transition the buffer back to be used
		GpuTransitionBarrier BufferBarrier = {};
		BufferBarrier.mBeforeState = D3D12_RESOURCE_STATE_COPY_DEST;
		BufferBarrier.mAfterState = InitialBufferState;
        CommandList->transitionBarrier(Result, BufferBarrier);
	}

	return Result;
}

GpuBuffer
GpuBuffer::createByteAdressBuffer(struct GpuFrameCache *FrameCache, GpuByteAddressBufferInfo &Info)
{
    u64 FrameSize  = Info.mCount * Info.mStride;
	u64 BufferSize = Info.mBufferFrames * FrameSize;

	// Not sure if this should be D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER or D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	GpuResource BufferResource = copyBuffer(
            FrameCache, Info.mData, BufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );

	GpuBuffer Result = {};
	Result.mType         = GpuBufferType::ByteAddress;
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
    FrameCache->trackResource(BufferResource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	return Result;
}

GpuBuffer
GpuBuffer::createIndexBuffer(struct GpuFrameCache *FrameCache, GpuIndexBufferInfo& Info)
{
    u64 Stride = Info.mIsU16 ? sizeof(u16)
                             : sizeof(u32);

    DXGI_FORMAT IndexFormat = (Info.mIsU16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	u64          BufferSize = Info.mIndexCount * Stride;

	GpuBuffer Result = {};
	Result.mType     = GpuBufferType::Index;
    Result.mResource = copyBuffer(FrameCache, Info.mIndices, BufferSize, D3D12_RESOURCE_FLAG_NONE,
                                  D3D12_RESOURCE_STATE_INDEX_BUFFER);

	Result.mViews.mIndexView.BufferLocation = Result.mResource.asHandle()->GetGPUVirtualAddress();
    Result.mViews.mIndexView.Format         = IndexFormat;
    Result.mViews.mIndexView.SizeInBytes    = (UINT)(BufferSize);

	Result.mStride = (u8)Stride;
	Result.mCount  = Info.mIndexCount;

    // Track the Resource state
    FrameCache->trackResource(Result.mResource, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	return Result;
}

GpuBuffer GpuBuffer::createStructuredBuffer(GpuFrameCache* FrameCache, GpuStructuredBufferInfo& Info)
{
    D3D12_RESOURCE_FLAGS  Flags              = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    u64 BufferSize                           = Info.mFrames * Info.mCount * Info.mStride;

    //copyBuffer(FrameCache, nullptr, u64 BufferSize, Flags, InitialBufferState)

    commited_resource_info ResourceInfo = {};
    ResourceInfo.Size                   = BufferSize;
    ResourceInfo.HeapType               = D3D12_HEAP_TYPE_UPLOAD;
    ResourceInfo.InitialState           = D3D12_RESOURCE_STATE_GENERIC_READ; // ...is this right?
    //ResourceInfo.ResourceFlags          = Flags;
    GpuResource Resource = FrameCache->getDevice()->createCommittedResource(ResourceInfo);
    Resource.asHandle()->SetName(L"PER MESH DATA");

    GpuBuffer Result    = {};
    Result.mType         = GpuBufferType::Structured;
    Result.mResource     = Resource;
    Result.mCpuVisible   = true;
    Result.mCount        = (u32)Info.mCount;
    Result.mStride       = (u32)Info.mStride;
    Result.mBufferFrames = Info.mFrames;

    // Track the Resource state
    FrameCache->trackResource(Resource, D3D12_RESOURCE_STATE_GENERIC_READ);

    return Result;
}

void GpuBuffer::map(u64 Frame)
{ // NOTE(enlynn): probably want to map only a small range, but being lazy for now...
    mResource.asHandle()->Map(0, nullptr, (void**)&mMappedData);
    mMappedFrame = (Frame % mBufferFrames);
    mFrameData   = (u8*)mMappedData + mMappedFrame * (mCount * mStride);
}

void GpuBuffer::unmap()
{
    mResource.asHandle()->Unmap(0, nullptr);
    mFrameData  = nullptr;
    mMappedData = nullptr;
    mMappedFrame = 0;
}