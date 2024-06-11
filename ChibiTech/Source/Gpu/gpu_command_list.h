#pragma once

#include <types.h>

#include "d3d12/d3d12.h"
#include "D3D12Common.h"
#include "gpu_device.h"
#include "gpu_resource.h"
#include "gpu_descriptor_allocator.h"
#include "gpu_dynamic_descriptor_heap.h"

#include <math/Math.h>
#include <Util/array.h>

class gpu_device;
class gpu_resource;

enum class gpu_command_list_type : u8
{
	none,     //error state
	graphics, //standard graphics command list
	compute,  //standard mCompute command list
	copy,     //standard copy command list
	indirect, //indirect command list
	count,
};

struct gpu_transition_barrier
{
	D3D12_RESOURCE_STATES BeforeState  = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_STATES AfterState   = D3D12_RESOURCE_STATE_COMMON;
	// Default value is to transition all subresources.
	u32                   Subresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
};

class gpu_command_list
{
public:
	gpu_command_list() = default;
	explicit gpu_command_list(gpu_device& Device, gpu_command_list_type Type);

	~gpu_command_list() { Release(); } // NOTE: I bet this will cause problems
	void Release();

	inline gpu_command_list_type             GetType()  const { return mType;   }
	inline struct ID3D12GraphicsCommandList* AsHandle() const { return mHandle; }

	void Reset();
	
	void Close();

    void BindRenderTarget(
        class gpu_render_target* RenderTarget,
        f32x4*                   ClearValue,
        bool                     ClearDepthStencil);

	// Texture Handling
	//void ClearTexture(const gpu_resource& TextureResource, f32 ClearColor[4]);

	// For now, assume all subresources should be 
	void TransitionBarrier(const gpu_resource& Resource, const gpu_transition_barrier& Barrier);

	// Requires GetCopyableFootprints to be called before passing paramters to this function
	u64 UpdateSubresources(
		const gpu_resource&                        DestinationResource,
		const gpu_resource&                        IntermediateResource,
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

	// The following two variations of UpdateSubresources will set up the D3D12_PLACED_SUBRESOURCE_FOOTPRINT
	// Array, but requires a set of arrays to be initialized based on the number of resources for the buffer.
	// These two functions stack allocate these arrays, so avoid using D3D12_REQ_SUBRESOURCES as the upper bound.
	template <u32 MaxSubresources>
    u64 UpdateSubresources(
		const gpu_resource&            DestinationResource,
		const gpu_resource&            IntermediateResource,
		u64                            IntermediateOffset,
		u32                            FirstSubresource,   // Range (0, MaxSubresources)
		u32                            NumSubresources,    // Range (1, MaxSubresources - FirstSubresource)
        farray<D3D12_SUBRESOURCE_DATA> SourceData          // Range List (Size = NumSubresources)
	) {
        u64                                RequiredSize = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MaxSubresources];
        UINT                               NumRows[MaxSubresources];
        UINT64                             RowSizesInBytes[MaxSubresources];
        
        D3D12_RESOURCE_DESC Desc = DestinationResource.GetResourceDesc();
        mDevice->AsHandle()->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset,
                                                   farray(Layouts, NumSubresources), farray(NumRows, NumSubresources),
                                                   farray(RowSizesInBytes, NumSubresources), &RequiredSize);

        return UpdateSubresources(
			DestinationResource, IntermediateResource, FirstSubresource, 
			NumSubresources, RequiredSize, farray(Layouts, NumSubresources), farray(NumRows, NumSubresources),
            farray(RowSizesInBytes, NumSubresources), SourceData);
    }

    // Frame Cache is required for State Transitions
    void CopyResource(struct gpu_frame_cache* FrameCache, const gpu_resource* DestinationResource, const gpu_resource* SourceResouce);

    void ResolveSubresource(struct gpu_frame_cache* FrameCache,          // Frame Cache is required for State Transitions
        const gpu_resource* DestinationResource, const gpu_resource* SourceResouce,
              u32 DestinationSubresource = 0,          u32 SourceSubresource = 0 );

	//Descriptor Binding
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* Heap);
	void BindDescriptorHeaps();

	// State Binding
	void SetGraphicsRootSignature(const gpu_root_signature& RootSignature);
	void SetScissorRect(D3D12_RECT& ScissorRect);
	void SetScissorRects(farray<D3D12_RECT> ScissorRects);
	void SetViewport(D3D12_VIEWPORT& Viewport);
	void SetViewports(farray<D3D12_VIEWPORT> Viewports);
	void SetPipelineState(const class gpu_pso& PipelineState);
	void SetTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

	// Buffer Binding
	void SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW IBView); // TODO(enlynn): Pass in a GpuBuffer

	// Resource View Bindings
	void SetShaderResourceView(u32 RootParameter, u32 DescriptorOffset, cpu_descriptor SRVDescriptor);
	void SetShaderResourceViewInline(u32 RootParameter, gpu_resource* Buffer, u64 BufferOffset = 0);

	void SetGraphics32BitConstants(u32 RootParameter, u32 NumConsants, void* Constants);
	template<typename T> void SetGraphics32BitConstants(u32 RootParameter, T* Constants)
	{
		SetGraphics32BitConstants(RootParameter, sizeof(T) / 4, (void*)Constants);
	}

	// Draw Commands
	void DrawInstanced(u32 VertexCountPerInstance, u32 InstanceCount = 1, u32 StartVertexLocation = 0, u32 StartInstanceLocation = 0);
	void DrawIndexedInstanced(u32 IndexCountPerInstance, u32 InstanceCount = 1, u32 StartIndexLocation = 0, u32 StartVertexLocation = 0, u32 StartInstanceLocation = 0);

private:
	gpu_command_list_type             mType                                                       = gpu_command_list_type::none;
	struct ID3D12GraphicsCommandList* mHandle                                                     = nullptr;
	struct ID3D12CommandAllocator*    mAllocator                                                  = nullptr;

	gpu_device*                       mDevice                                                     = nullptr;

	ID3D12DescriptorHeap*             mBoundDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
	gpu_dynamic_descriptor_heap       mDynamicDescriptors[u32(dynamic_heap_type::max)]            = {};

	ID3D12PipelineState*              mBoundPipeline                                              = nullptr;
	ID3D12RootSignature*              mBoundRootSignature                                         = nullptr;
};
