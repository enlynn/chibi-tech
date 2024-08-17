#include "GpuCommandList.h"

#include "GpuPso.h"
#include "GpuRootSignature.h"
#include "GpuRenderTarget.h"
#include "GpuState.h"
#include "GpuResourceViews.h"

#include <Platform/Assert.h>
#include <Platform/Console.h>

inline void 
MemcpySubresource(
	const D3D12_MEMCPY_DEST*      Destination,
	const D3D12_SUBRESOURCE_DATA* Source,
	u64                           RowSizeInBytes,
	u32                           NumRows,
	u32                           NumSlices)
{
	ForRange (UINT, z, NumSlices)
	{
		auto pDestSlice = (BYTE*)(Destination->pData) + Destination->SlicePitch * z;
		auto pSrcSlice = (const BYTE*)(Source->pData) + Source->SlicePitch * LONG_PTR(z);
		for (UINT y = 0; y < NumRows; ++y)
		{
			memcpy(
				pDestSlice + Destination->RowPitch * y,
				pSrcSlice + Source->RowPitch * LONG_PTR(y),
				RowSizeInBytes
			);
		}
	}
}

inline D3D12_TEXTURE_COPY_LOCATION
GetTextureCopyLoction(ID3D12Resource* Resource, u32 SubresourceIndex)
{
	D3D12_TEXTURE_COPY_LOCATION Result = {};
	Result.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	Result.PlacedFootprint  = {};
	Result.SubresourceIndex = SubresourceIndex;
	Result.pResource        = Resource;
	return Result;
}

inline D3D12_TEXTURE_COPY_LOCATION
GetTextureCopyLoction(ID3D12Resource* Resource, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& Footprint)
{
	D3D12_TEXTURE_COPY_LOCATION Result = {};
	Result.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	Result.PlacedFootprint = Footprint;
	Result.pResource       = Resource;
	return Result;
}

inline D3D12_COMMAND_LIST_TYPE
ToD3D12CommandListType(GpuCommandListType Type)
{
#if 0
D3D12_COMMAND_LIST_TYPE_DIRECT = 0,
D3D12_COMMAND_LIST_TYPE_BUNDLE = 1,
D3D12_COMMAND_LIST_TYPE_COMPUTE = 2,
D3D12_COMMAND_LIST_TYPE_COPY = 3,
D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE = 4,
D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS = 5,
D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE = 6,
D3D12_COMMAND_LIST_TYPE_NONE = -1
#endif

	switch (Type)
	{
		case GpuCommandListType::Graphics: // intentional fallthrough
		case GpuCommandListType::Indirect: return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case GpuCommandListType::Compute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case GpuCommandListType::Copy:     return D3D12_COMMAND_LIST_TYPE_COPY;
		default:                              return D3D12_COMMAND_LIST_TYPE_NONE;
		
	}
}

GpuCommandList::GpuCommandList(GpuDeviceSPtr Device, GpuCommandListType Type)
	: mType(Type)
	, mDevice(Device)
{
	const D3D12_COMMAND_LIST_TYPE d3D12Type = ToD3D12CommandListType(Type);

	AssertHr(Device->asHandle()->CreateCommandAllocator(d3D12Type, ComCast(&mAllocator)));
	AssertHr(Device->asHandle()->CreateCommandList(0, d3D12Type, mAllocator, nullptr, ComCast(&mHandle)));

	// A command list is created in the recording state, but since the Command Queue is responsible for fetching
	// a command list, it is ok for the list to stay in this state.

	ForRange(int, i, u32(DynamicHeapType::Max))
	{
		mDynamicDescriptors[i] = GpuDynamicDescriptorHeap(Device, DynamicHeapType(i), 1024);
	}
}

void GpuCommandList::release()
{
	ForRange(int, i, u32(DynamicHeapType::Max))
	{
		mDynamicDescriptors[i].deinit();
	}

	if (mAllocator)
		ComSafeRelease(mAllocator);
	if (mHandle)
		ComSafeRelease(mHandle);
}

void GpuCommandList::reset()
{
	AssertHr(mAllocator->Reset());
	AssertHr(mHandle->Reset(mAllocator, nullptr));

	ForRange(int, i, u32(DynamicHeapType::Max))
	{
        mDynamicDescriptors[i].reset();
	}

    ForRange(int, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
    {
        mBoundDescriptorHeaps[i] = nullptr;
    }

	mBoundPipeline      = nullptr;
	mBoundRootSignature = nullptr;
}

void GpuCommandList::close()
{
	mHandle->Close();
}

void 
GpuCommandList::transitionBarrier(const GpuResource& Resource, const GpuTransitionBarrier& Barrier)
{
	D3D12_RESOURCE_BARRIER TransitionBarrier = {};
	TransitionBarrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	TransitionBarrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	TransitionBarrier.Transition.pResource   = Resource.asHandle();
	TransitionBarrier.Transition.StateBefore = Barrier.mBeforeState; // TODO: I don't think this accurate, but let's see what happens...
	TransitionBarrier.Transition.StateAfter  = Barrier.mAfterState;
	TransitionBarrier.Transition.Subresource = Barrier.mSubresources;

	// TODO(enlynn): Ideally I wouldn't immediately flush the barriers, but since I haven't written
	// ResourceState management, just flush the barriers immediately.
	mHandle->ResourceBarrier(1, &TransitionBarrier);
}

u64 
GpuCommandList::updateSubresources(
	const GpuResource&                        DestinationResource,
	const GpuResource&                        IntermediateResource,
	u32                                        FirstSubresource,   // Range = (0,D3D12_REQ_SUBRESOURCES)
	u32                                        NumSubresources,    // Range = (0,D3D12_REQ_SUBRESOURCES-FirstSubresource)
	u64                                        RequiredSize,
	farray<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> SubresourceLayouts, // Size == NumSubresources
	farray<u32>                                NumRows,            // Size == NumSubresources
	farray<u64>                                NumRowSizesInBytes, // Size == NumSubresources
	farray<D3D12_SUBRESOURCE_DATA>             SubresourceData     // Size == NumSubresources
) {
	// Quick error checking.
	if (FirstSubresource > D3D12_REQ_SUBRESOURCES)
	{
		ct::console::fatal("GpuCommandList::updateSubresources : First Subresource should be between (0, 30720), but %d was provided.", FirstSubresource);
	}

	if (NumSubresources > (D3D12_REQ_SUBRESOURCES - FirstSubresource))
	{
        ct::console::fatal("GpuCommandList::updateSubresources : Number of mSubresources should be between (FirstSubresource, 30720), but %d was provided.", NumSubresources);
	}

	if (SubresourceLayouts.Length() > 0 && SubresourceLayouts.Length() != NumSubresources)
	{
        ct::console::fatal("GpuCommandList::updateSubresources : Number of Subresource Layouts should be the number of subresources to update, but was %d.", SubresourceLayouts.Length());
	}

	if (SubresourceLayouts.Length() > 0 && NumRows.Length() != NumSubresources)
	{
        ct::console::fatal("GpuCommandList::updateSubresources : Number of Subresource Rows should be the number of subresources to update, but was %d.", NumRows.Length());
	}

	if (SubresourceLayouts.Length() > 0 && NumRowSizesInBytes.Length() != NumSubresources)
	{
        ct::console::fatal("GpuCommandList::updateSubresources : Number of Subresource Row Sizes should be the number of subresources to update, but was %d.", NumRowSizesInBytes.Length());
	}

	if (SubresourceLayouts.Length() > 0 && SubresourceData.Length() != NumSubresources)
	{
        ct::console::fatal("GpuCommandList::updateSubresources : Number of Subresource Datas should be the number of subresources to update, but was %d.", SubresourceData.Length());
	}

	// Some more minor validation
	D3D12_RESOURCE_DESC IntermediateDesc = IntermediateResource.getResourceDesc();
	D3D12_RESOURCE_DESC DestinationDesc  = DestinationResource.getResourceDesc();

	if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
		IntermediateDesc.Width < RequiredSize + SubresourceLayouts[u64(0)].Offset ||
		RequiredSize > u64(-1) ||
		(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
			(FirstSubresource != 0 || NumSubresources != 1)))
	{
		return 0;
	}

	ID3D12Resource* pIntermediate = IntermediateResource.asHandle();
	ID3D12Resource* pDestination  = DestinationResource.asHandle();

	BYTE* pData;
	HRESULT hr = pIntermediate->Map(0, nullptr, (void**)(&pData));
	if (FAILED(hr))
	{
		return 0;
	}

	ForRange (UINT, i, NumSubresources)
	{
		if (NumRowSizesInBytes[i] > SIZE_T(-1)) 
			return 0;

		D3D12_MEMCPY_DEST DestData = { 
			pData + SubresourceLayouts[i].Offset, 
			SubresourceLayouts[i].Footprint.RowPitch, 
			SIZE_T(SubresourceLayouts[i].Footprint.RowPitch) * SIZE_T(NumRows[i]) 
		};

		MemcpySubresource(
			&DestData, &SubresourceData[i], (SIZE_T)(NumRowSizesInBytes[i]), 
			NumRows[i], SubresourceLayouts[i].Footprint.Depth);
	}
	pIntermediate->Unmap(0, nullptr);

	if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		u64 SubresourceOffset = SubresourceLayouts[(u64)0].Offset;
		u32 SubresourceWidth  = SubresourceLayouts[(u64)0].Footprint.Width;
		mHandle->CopyBufferRegion(pDestination, 0, pIntermediate, SubresourceOffset, SubresourceWidth);
	}
	else
	{
		ForRange (UINT, i, NumSubresources)
		{
			D3D12_TEXTURE_COPY_LOCATION Dst = GetTextureCopyLoction(pDestination, i + FirstSubresource);
			D3D12_TEXTURE_COPY_LOCATION Src = GetTextureCopyLoction(pIntermediate, SubresourceLayouts[i]);
			mHandle->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
	}
	return RequiredSize;
}

void GpuCommandList::resolveSubresource(struct GpuFrameCache *FrameCache,
                                        const GpuResource* DestinationResource, const GpuResource* SourceResouce,
                                        u32 DestinationSubresource, u32 SourceSubresource)
{
    FrameCache->transitionResource(DestinationResource, D3D12_RESOURCE_STATE_RESOLVE_DEST, DestinationSubresource);
    FrameCache->transitionResource(SourceResouce, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, SourceSubresource);

    FrameCache->flushResourceBarriers(this);

    mHandle->ResolveSubresource(DestinationResource->asHandle(), DestinationSubresource,
                                SourceResouce->asHandle(), SourceSubresource,
                                DestinationResource->getResourceDesc().Format);
}

void 
GpuCommandList::setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* Heap)
{
	if (mBoundDescriptorHeaps[Type] != Heap)
	{
		mBoundDescriptorHeaps[Type] = Heap;
        bindDescriptorHeaps();
	}
}

void
GpuCommandList::bindDescriptorHeaps()
{
	UINT                  NumHeaps                                          = 0;
	ID3D12DescriptorHeap* HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	ForRange(int, i, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES)
	{
		if (mBoundDescriptorHeaps[i] != nullptr)
		{
			HeapsToBind[NumHeaps] = mBoundDescriptorHeaps[i];
			NumHeaps             += 1;
		}
	}

	mHandle->SetDescriptorHeaps(NumHeaps, HeapsToBind);
}

void GpuCommandList::setIndexBuffer(D3D12_INDEX_BUFFER_VIEW IBView)
{
	//transitionBarrier(indexBuffer->getResource(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
	mHandle->IASetIndexBuffer(&IBView);
}

void
GpuCommandList::setGraphics32BitConstants(u32 RootParameter, u32 NumConsants, void* Constants)
{
	mHandle->SetGraphicsRoot32BitConstants(RootParameter, NumConsants, Constants, 0);
}

// Set the SRV on the Graphics pipeline.
void
GpuCommandList::setShaderResourceView(u32 RootParameter, u32 DescriptorOffset, CpuDescriptor SRVDescriptor)
{
	// TODO(enlynn): Verify the Resource is in the correct final state
    mDynamicDescriptors[u32(DynamicHeapType::Buffer)].stageDescriptors(RootParameter, DescriptorOffset, 1,
                                                                       SRVDescriptor.getDescriptorHandle());
}

void GpuCommandList::setShaderResourceViewInline(u32 RootParameter, GpuResource* Buffer, u64 BufferOffset)
{
	if (Buffer)
	{
        mDynamicDescriptors[u32(DynamicHeapType::Buffer)].stageInlineSrv(RootParameter, Buffer->getGpuAddress() + BufferOffset);
	}
}


void GpuCommandList::setShaderResourceView(u32 tRootParameterIndex, u32 tDescriptorOffset, GpuShaderResourceView &tSrv) {
    stageDynamicDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tRootParameterIndex, tDescriptorOffset, 1, tSrv.getDescriptorHandle());
}

void GpuCommandList::setShaderResourceView(u32 tRootParameterIndex, u32 tDescriptorOffset, GpuTexture &tTexture) {
    stageDynamicDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tRootParameterIndex, tDescriptorOffset, 1, tTexture.getShaderResourceView().getDescriptorHandle());
}

void GpuCommandList::setUnorderedAccessView(u32 tRootParameterIndex, u32 tDescriptorOffset, GpuUnorderedAccessView &tUav) {
    stageDynamicDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, tRootParameterIndex, tDescriptorOffset, 1, tUav.getDescriptorHandle());
}

void GpuCommandList::stageDynamicDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE tType, u32 tRootParameterIndex, u32 tDescriptorOffset,
                                             u32 tNumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE tCpuDescriptorHandle) {

    u32 tIndex = 0;
    switch (tType) {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: tIndex = u32(DynamicHeapType::Buffer);  break;
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:     tIndex = u32(DynamicHeapType::Sampler); break;
        default:
            ASSERT_CUSTOM(false, "Invalid Descriptor Heap Type for staging dynamic descriptors."); break;
    }

    mDynamicDescriptors[tIndex].stageDescriptors(tRootParameterIndex, tDescriptorOffset, tNumDescriptors, tCpuDescriptorHandle);
}

void GpuCommandList::setGraphicsRootSignature(const GpuRootSignature& RootSignature)
{
	ID3D12RootSignature* RootHandle = RootSignature.asHandle();
	if (mBoundRootSignature != RootHandle)
	{
		mBoundRootSignature = RootHandle;

		ForRange(int, i, u32(DynamicHeapType::Max))
		{
            mDynamicDescriptors[i].parseRootSignature(RootSignature);
		}

		mHandle->SetGraphicsRootSignature(mBoundRootSignature);
	}
}

void GpuCommandList::setComputeRootSignature(const GpuRootSignature& RootSignature) {
    ID3D12RootSignature* RootHandle = RootSignature.asHandle();
    if (mBoundRootSignature != RootHandle)
    {
        mBoundRootSignature = RootHandle;

        ForRange(int, i, u32(DynamicHeapType::Max))
        {
            mDynamicDescriptors[i].parseRootSignature(RootSignature);
        }

        mHandle->SetComputeRootSignature(mBoundRootSignature);
    }
}

void
GpuCommandList::setScissorRect(D3D12_RECT& ScissorRect)
{
    setScissorRects(farray(&ScissorRect, 1));
}

void
GpuCommandList::setScissorRects(farray<D3D12_RECT> ScissorRects)
{
	ASSERT(ScissorRects.Length() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
	mHandle->RSSetScissorRects((UINT)ScissorRects.Length(), ScissorRects.Ptr());
}

void
GpuCommandList::setViewport(D3D12_VIEWPORT& Viewport)
{
    setViewports(farray(&Viewport, 1));
}

void
GpuCommandList::setViewports(farray<D3D12_VIEWPORT> Viewports)
{
    ASSERT(Viewports.Length() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
	mHandle->RSSetViewports((UINT)Viewports.Length(), Viewports.Ptr());
}

void
GpuCommandList::setPipelineState(const GpuPso &PipelineState)
{
	if (mBoundPipeline != PipelineState.asHandle())
	{
		mBoundPipeline = PipelineState.asHandle();
		mHandle->SetPipelineState(mBoundPipeline);
	}
}

void
GpuCommandList::setTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
{
	mHandle->IASetPrimitiveTopology(Topology);
}

void
GpuCommandList::drawInstanced(u32 VertexCountPerInstance, u32 InstanceCount, u32 StartVertexLocation, u32 StartInstanceLocation)
{
	//flushResourceBarriers();

	ForRange (int, i, u32(DynamicHeapType::Max))
	{
        mDynamicDescriptors[i].commitStagedDescriptorsForDraw(this);
	}

	mHandle->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}


void
GpuCommandList::drawIndexedInstanced(u32 IndexCountPerInstance, u32 InstanceCount, u32 StartIndexLocation, u32 StartVertexLocation, u32 StartInstanceLocation)
{
	//flushResourceBarriers();

	ForRange(int, i, u32(DynamicHeapType::Max))
	{
        mDynamicDescriptors[i].commitStagedDescriptorsForDraw(this);
	}

	mHandle->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, StartVertexLocation, StartInstanceLocation);
}

void GpuCommandList::bindRenderTarget(
        struct GpuRenderTarget *RenderTarget,
        float4*             ClearValue,
        bool               ClearDepthStencil)
{
    D3D12_CPU_DESCRIPTOR_HANDLE RTHandles[int(AttachmentPoint::Count)];
    u32 RTHandlesCount = 0;

    // Transition all textures to Render Target
    ForRange(int, i, int(AttachmentPoint::DepthStencil))
    {
        const GpuTexture* Framebuffer = RenderTarget->getTexture(AttachmentPoint(i));
        if (Framebuffer)
        {
            CpuDescriptor FramebufferRTV = Framebuffer->getRenderTargetView();

            RTHandles[RTHandlesCount] = FramebufferRTV.getDescriptorHandle();
            RTHandlesCount += 1;

            if (ClearValue)
                mHandle->ClearRenderTargetView(FramebufferRTV.getDescriptorHandle(), ClearValue->Ptr, 0, nullptr);
        }
    }

    // Clear Depth-Stencil if asked for and has a valid Depth Texture
    D3D12_CPU_DESCRIPTOR_HANDLE  DSView       = {};
    D3D12_CPU_DESCRIPTOR_HANDLE* DSViewHandle = nullptr;

    const GpuTexture* DepthTexture    = RenderTarget->getTexture(AttachmentPoint::DepthStencil);
    if (DepthTexture)
    {
        DSView       = DepthTexture->getDepthStencilView().getDescriptorHandle();
        DSViewHandle = &DSView;

        if (ClearDepthStencil)
        {
            D3D12_CLEAR_FLAGS Flags = D3D12_CLEAR_FLAG_DEPTH;

            // Check to see if it is also has stencil
            D3D12_RESOURCE_DESC Desc = DepthTexture->getResourceDesc();
            if (Desc.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT || Desc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT)
                Flags |= D3D12_CLEAR_FLAG_STENCIL;

            // Hard Code the Depth/Stencil Clear Values for now, will pull out if need to manually specify
            const float Depth   = 1.0f;
            const int   Stencil = 0;

            mHandle->ClearDepthStencilView(DSView, Flags, Depth, Stencil, 0, nullptr);
        }
    }

    mHandle->OMSetRenderTargets(RTHandlesCount, RTHandles, FALSE, DSViewHandle);
}

void GpuCommandList::setCompute32BitConstants(u32 tRootParameterIndex, u32 tNumConstants, const void *tConstants) {
    mHandle->SetComputeRoot32BitConstants(tRootParameterIndex, tNumConstants, tConstants, 0);
}

void GpuCommandList::dispatch(u32 tNumGroupsX, u32 tNumGroupsY, u32 tNumGroupsZ) {
    ForRange(int, i, u32(DynamicHeapType::Max)) {
        mDynamicDescriptors[i].commitStagedDescriptorsForDispatch(this);
    }

    mHandle->Dispatch(tNumGroupsX, tNumGroupsY, tNumGroupsZ);
}


void GpuCommandList::copyResource(struct GpuFrameCache *FrameCache, const GpuResource* DestinationResource, const GpuResource* SourceResouce)
{
    FrameCache->transitionResource(DestinationResource, D3D12_RESOURCE_STATE_COPY_DEST);
    FrameCache->transitionResource(SourceResouce, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FrameCache->flushResourceBarriers(this);

    copyResource(*DestinationResource, *SourceResouce);
}

void GpuCommandList::copyResource(const GpuResource &tDstRes, const GpuResource &tSrcRes) {
    mHandle->CopyResource(tDstRes.asHandle(), tSrcRes.asHandle());
}


