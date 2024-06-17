#pragma once

#include <Types.h>

#include "d3d12/d3d12.h"
#include "D3D12Common.h"
#include "GpuDevice.h"
#include "GpuResource.h"
#include "GpuDescriptorAllocator.h"
#include "GpuDynamicDescriptorHeap.h"

#include <Math/Math.h>
#include <Util/array.h>

class GpuDevice;
class GpuResource;
class GpuPso;
class GpuShaderResourceView;
class GpuUnorderedAccessView;
class GpuTexture;

enum class GpuCommandListType : u8
{
	None,     //error state
	Graphics, //standard Graphics command list
	Compute,  //standard mCompute command list
	Copy,     //standard copy command list
	Indirect, //indirect command list
	Count,
};

struct GpuTransitionBarrier
{
	D3D12_RESOURCE_STATES mBeforeState  = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES mAfterState   = D3D12_RESOURCE_STATE_COMMON;
	// Default value is to transition all subresources.
	u32                   mSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
};

class GpuCommandList
{
public:
	GpuCommandList() = default;
	explicit GpuCommandList(GpuDevice& Device, GpuCommandListType Type);

	~GpuCommandList() { release(); } // NOTE: I bet this will cause problems
	void release();

	inline GpuCommandListType                getType()  const { return mType;   }
	inline struct ID3D12GraphicsCommandList* asHandle() const { return mHandle; }

	void reset();
	
	void close();

    void bindRenderTarget(
            struct GpuRenderTarget *RenderTarget,
            float4*                   ClearValue,
            bool                     ClearDepthStencil);

	// Texture Handling
	//void ClearTexture(const GpuResource& TextureResource, f32 ClearColor[4]);

	// For now, assume all subresources should be 
	void transitionBarrier(const GpuResource& Resource, const GpuTransitionBarrier& Barrier);

	// Requires GetCopyableFootprints to be called before passing paramters to this function
	u64 updateSubresources(
		const GpuResource&                        DestinationResource,
		const GpuResource&                        IntermediateResource,
		u32                                        FirstSubresource,   // Range = (0,D3D12_REQ_SUBRESOURCES)
		u32                                        NumSubresources,    // Range = (0,D3D12_REQ_SUBRESOURCES-FirstSubresource)
		u64                                        RequiredSize,
		farray<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> SubresourceLayouts, // Size == NumSubresources
        farray<u32>                                NumRows,            // Size == NumSubresources
        farray<u64>                                NumRowSizesInBytes, // Size == NumSubresources
        farray<D3D12_SUBRESOURCE_DATA>             SubresourceData     // Size == NumSubresources
	);

	// NOTE(enlynn):
	// I am unsure about these template variations. I feel like in most situations
	// the MaxSubresources will not be known at compile time. Will keep this version 
	// until this is no longer true.

	// The following two variations of updateSubresources will set up the D3D12_PLACED_SUBRESOURCE_FOOTPRINT
	// Array, but requires a set of arrays to be initialized based on the number of resources for the buffer.
	// These two functions stack allocate these arrays, so avoid using D3D12_REQ_SUBRESOURCES as the upper bound.
	template <u32 MaxSubresources>
    u64 updateSubresources(
		const GpuResource&            DestinationResource,
		const GpuResource&            IntermediateResource,
		u64                            IntermediateOffset,
		u32                            FirstSubresource,   // Range (0, MaxSubresources)
		u32                            NumSubresources,    // Range (1, MaxSubresources - FirstSubresource)
        farray<D3D12_SUBRESOURCE_DATA> SourceData          // Range List (Size = NumSubresources)
	) {
        u64                                RequiredSize = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MaxSubresources];
        UINT                               NumRows[MaxSubresources];
        UINT64                             RowSizesInBytes[MaxSubresources];
        
        D3D12_RESOURCE_DESC Desc = DestinationResource.getResourceDesc();
        mDevice->asHandle()->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset,
                                                   farray(Layouts, NumSubresources), farray(NumRows, NumSubresources),
                                                   farray(RowSizesInBytes, NumSubresources), &RequiredSize);

        return updateSubresources(
                DestinationResource, IntermediateResource, FirstSubresource,
                NumSubresources, RequiredSize, farray(Layouts, NumSubresources), farray(NumRows, NumSubresources),
                farray(RowSizesInBytes, NumSubresources), SourceData);
    }

    // Frame Cache is required for State Transitions
    void copyResource(struct GpuFrameCache *FrameCache, const GpuResource* DestinationResource, const GpuResource* SourceResouce);
    void copyResource(const GpuResource& tDstRes, const GpuResource& tSrcRes);

    void resolveSubresource(struct GpuFrameCache *FrameCache,          // Frame Cache is required for State Transitions
        const GpuResource* DestinationResource, const GpuResource* SourceResouce,
                            u32 DestinationSubresource = 0, u32 SourceSubresource = 0);

	//Descriptor Binding
	void setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* Heap);
	void bindDescriptorHeaps();

	// State Binding
	void setGraphicsRootSignature(const GpuRootSignature& RootSignature);
	void setScissorRect(D3D12_RECT& ScissorRect);
	void setScissorRects(farray<D3D12_RECT> ScissorRects);
	void setViewport(D3D12_VIEWPORT& Viewport);
	void setViewports(farray<D3D12_VIEWPORT> Viewports);
	void setPipelineState(const GpuPso &PipelineState);
	void setTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

    // Descriptors
    void stageDynamicDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE tType, u32 tRootParameterIndex, u32 tDescriptorOffset,
                                 u32 tNumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE tCpuDescriptorHandle);

    // Compute
    void setComputeRootSignature(const GpuRootSignature& RootSignature);

    void setCompute32BitConstants(u32 tRootParameterIndex, u32 tNumConstants, const void* tConstants);
    template<typename T>
    void setCompute32BitConstants(u32 tRootParameterIndex, const T& tConstants)
    {
        static_assert( sizeof( T ) % sizeof( u32 ) == 0, "Size of type must be a multiple of 4 bytes" );
        setCompute32BitConstants(tRootParameterIndex, sizeof( T ) / sizeof( u32 ), &tConstants);
    }

	// Buffer Binding
	void setIndexBuffer(D3D12_INDEX_BUFFER_VIEW IBView); // TODO(enlynn): Pass in a GpuBuffer

	// Resource View Bindings
	void setShaderResourceView(u32 RootParameter, u32 DescriptorOffset, CpuDescriptor SRVDescriptor);
	void setShaderResourceViewInline(u32 RootParameter, GpuResource* Buffer, u64 BufferOffset = 0);
    void setShaderResourceView(u32 tRootParameterIndex, u32 tDescriptorOffset, GpuShaderResourceView& tSrv);
    void setShaderResourceView(u32 tRootParameterIndex, u32 tDescriptorOffset, GpuTexture& tTexture);

    // Unordered Access Views
    void setUnorderedAccessView(u32 tRootParameterIndex, u32 tDescriptorOffset, GpuUnorderedAccessView& tUav);

	void setGraphics32BitConstants(u32 RootParameter, u32 NumConsants, void* Constants);
	template<typename T> void setGraphics32BitConstants(u32 RootParameter, T* Constants)
	{
        setGraphics32BitConstants(RootParameter, sizeof(T) / 4, (void *) Constants);
	}

	// Draw Commands
	void drawInstanced(u32 VertexCountPerInstance, u32 InstanceCount = 1, u32 StartVertexLocation = 0, u32 StartInstanceLocation = 0);
	void drawIndexedInstanced(u32 IndexCountPerInstance, u32 InstanceCount = 1, u32 StartIndexLocation = 0, u32 StartVertexLocation = 0, u32 StartInstanceLocation = 0);

    // Compute Draw Commands
    void dispatch(u32 tNumGroupsX, u32 tNumGroupsY = 1, u32 tNumGroupsZ = 1);

private:
	GpuCommandListType                mType                                                       = GpuCommandListType::None;
	struct ID3D12GraphicsCommandList* mHandle                                                     = nullptr;
	struct ID3D12CommandAllocator*    mAllocator                                                  = nullptr;

	GpuDevice*                        mDevice                                                     = nullptr;

	ID3D12DescriptorHeap*             mBoundDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
	GpuDynamicDescriptorHeap          mDynamicDescriptors[u32(DynamicHeapType::Max)]              = {};

	ID3D12PipelineState*              mBoundPipeline                                              = nullptr;
	ID3D12RootSignature*              mBoundRootSignature                                         = nullptr;
};
